#include "sectordescriptor.h"
#include "block.h"
#include "pid.h"
#include "freesectordescriptorstore.h"
#include "diskdevice.h"
#include "voucher.h"
#include "BoundedBuffer.h"
#include <pthread.h>
#include "freesectordescriptorstore_full.h"
#include "sectordescriptorcreator.h"
#include "diagnostics.h"
#include <stdio.h>

#define NUM_VOUCHERS 30
#define BUFFER_SIZE 17

/* Voucher Data Structure */
struct voucher{
	int finished, frw, success; /* flags: finished (0, 1), free/read/write( 0, 1, 2), success (0, 1) */
	SectorDescriptor *sd; /*contained sector descriptor data*/
	//each voucher has its own mutex and condition
	pthread_cond_t usable; 
	pthread_mutex_t inUse;
};

Voucher vouchers[NUM_VOUCHERS]; //will no have more than 30 vouchers

DiskDevice *diskDev; //global reference to the disk device

BoundedBuffer *write_b, *read_b, *voucher_b; //global buffer references
pthread_t writer_th, reader_th; //global thread declarations

FreeSectorDescriptorStore *FSDS;//global free sector descriptor store reference


/*definitions of functions required for threads*/
/* Writer thread; continually check if there is anything in the write queue */
void *writer_f(){
	Voucher *v;
	while(1){
		v = (Voucher *)blockingReadBB(write_b); //we want to guarantee that we will have a voucher to operate on

		pthread_mutex_lock(&v->inUse);

		v->success = write_sector(diskDev, v->sd); //returns a 1/0 corresponding to success or not, so we can use this for assignment
		if(v->success == 1){
			printf("[DRIVER> %d successful write of sector %d to disk\n", (int)sector_descriptor_get_pid(v->sd), (int)sector_descriptor_get_block(v->sd));
		}
		else{
			printf("[DRIVER> %d failed write of sector %d to disk\n", (int)sector_descriptor_get_pid(v->sd), (int)sector_descriptor_get_block(v->sd));
		}
		v->finished = 1; //we are done with the voucher
		v->frw = 0; //reset flag to denote that voucher is free
		blockingWriteBB(voucher_b, v);

		blocking_put_sd(FSDS, v->sd);

		pthread_cond_signal(&v->usable);
		pthread_mutex_unlock(&v->inUse);
	}
}
/* Reader thread; continually check if there is anything to be read */
void *reader_f(){
	Voucher *v;
	while(1){
		v = (Voucher *)blockingReadBB(read_b); //we want to guarantee that we will have a voucher to operate on
		pthread_mutex_lock(&v->inUse);

		v->success = read_sector(diskDev, v->sd); //returns a 1/0 corresponding to success or not, so we can use this for assignment
		if(v->success == 1){
			printf("[DRIVER> %d successful read of sector %d from disk\n",(int)sector_descriptor_get_pid(v->sd), (int)sector_descriptor_get_block(v->sd));
		}
		else{
			printf("[DRIVER> %d failed read of sector %d from disk\n",(int)sector_descriptor_get_pid(v->sd),(int)sector_descriptor_get_block(v->sd));
		}
		v->finished = 1; //regardless, we are done with the voucher
		v->frw = 0; //reset flag to denote that voucher is free
		blockingWriteBB(voucher_b, v);

		pthread_cond_signal(&v->usable);
		pthread_mutex_unlock(&v->inUse);
	}
}


/*
 * the following calls must be implemented by the students
 */

/*
 * called before any other methods to allow you to initialize data
 * structures and to start any internal threads.
 *
 * Arguments:
 *   dd: the DiskDevice that you must drive
 *   mem_start, mem_length: some memory for SectorDescriptors
 *   fsds_ptr: you hand back a FreeSectorDescriptorStore constructed
 *             from the memory provided in the two previous arguments
 */
void init_disk_driver(DiskDevice *dd, void *mem_start, unsigned long mem_length, FreeSectorDescriptorStore **fsds){
	/* create Free Sector Descriptor Store */
	diskDev = dd;
	FSDS = create_fsds();
	/* load FSDS with packet descriptors constructed from mem_start/mem_length */
	create_free_sector_descriptors(FSDS, mem_start, mem_length);
	/* create buffers required by threads */
	write_b = createBB(BUFFER_SIZE);
	read_b = createBB(BUFFER_SIZE);
	voucher_b = createBB(NUM_VOUCHERS);
	//create number of vouchers for the sector descriptors
	int i;
	for (i=0; i<NUM_VOUCHERS; i++){
		vouchers[i].sd = NULL;
		vouchers[i].finished = 0;
		vouchers[i].frw = 0;
		vouchers[i].success = 0;
		pthread_cond_init(&vouchers[i].usable, NULL);
		pthread_mutex_init(&vouchers[i].inUse, NULL);
		blockingWriteBB(voucher_b, &vouchers[i]);
	}
	/* create threads required for implementation */
	if(pthread_create (&writer_th, NULL, &writer_f, NULL) != 0){
		printf("[DRIVER> Error creating writer thread\n");
		return;
	}
	if(pthread_create (&reader_th, NULL, &reader_f, NULL) != 0){
		printf("[DRIVER> Error creating reader thread\n");
		return;
	}
	/* return the FSDS to the code that called you */
	*fsds = FSDS;
}

/*
 * the following calls are used to write a sector to the disk
 * the nonblocking call must return promptly, returning 1 if successful at
 * queueing up the write, 0 if not (in case internal buffers are full)
 * the blocking call will usually return promptly, but there may be
 * a delay while it waits for space in your buffers.
 * neither call should delay until the sector is actually written to the disk
 * for a successful nonblocking call and for the blocking call, a voucher is
 * returned that is required to determine the success/failure of the write
 */
void blocking_write_sector(SectorDescriptor *sd, Voucher **v){
	/* queue up sector descriptor for writing */
	/* return a Voucher through *v for eventual synchronization by application */
	/* do not return until it has been successfully queued */
	Voucher *p = blockingReadBB(voucher_b);
	pthread_mutex_lock(&p->inUse);
	p->frw = 2;
	p->sd = sd;
	p->finished = 0;
	blockingWriteBB(write_b, p);
	*v = p;
	pthread_cond_signal(&p->usable);
	pthread_mutex_unlock(&p->inUse);
	return;
}
int nonblocking_write_sector(SectorDescriptor *sd, Voucher **v){
	/* if you are able to queue up sector descriptor immediately: */
	/* return a voucher through *v and return 1 */
	/* otherwise return 0 */
	//if we don't get a voucher right off the bat, don't bother doing anything
	if(nonblockingReadBB(voucher_b, (void**)v) == 0){
		return 0;
	}
	//otherwise, we can start doing things with it!
	Voucher *p = *v;
	pthread_mutex_lock(&p->inUse);
	p->frw = 2;
	p->sd = sd;
	if(nonblockingWriteBB(write_b, p) == 1){
		p->finished = 0;
		*v = p;
		pthread_cond_signal(&p->usable);
		pthread_mutex_unlock(&p->inUse);
		return 1;
	}
	//if the nonblocking write was unsuccessful; 
	//we will need to return the voucher state and put it back into the buffer
	p->frw = 0;
	p->sd = NULL;
	blockingWriteBB(voucher_b, p);
	pthread_cond_signal(&p->usable);
	pthread_mutex_unlock(&p->inUse);
	*v = NULL;
	return 0;
}

/*
 * the following calls are used to initiate the read of a sector from the disk
 * the nonblocking call must return promptly, returning 1 if successful at
 * queueing up the read, 0 if not (in case internal buffers are full)
 * the blocking callwill usually return promptly, but there may be
 * a delay while it waits for space in your buffers.
 * neither call should delay until the sector is actually read from the disk
 * for successful nonblocking call and for the blocking call, a voucher is
 * returned that is required to collect the sector after the read completes.
 */
void blocking_read_sector(SectorDescriptor *sd, Voucher **v){
	/* queue up sector descriptor for reading */
	/* return a Voucher through *v for eventual synchronization by application */
	/* do not return until it has been successfully queued */

	//Acquire a voucher
	Voucher *p = blockingReadBB(voucher_b);
	//we are guaranteed a voucher from blocking read
	pthread_mutex_lock(&p->inUse);
	p->frw = 1;
	p->sd = sd;
	p->finished = 0;
	blockingWriteBB(read_b, p);
	*v = p;
	pthread_cond_signal(&p->usable);
	pthread_mutex_unlock(&p->inUse);
	return;
}
int nonblocking_read_sector(SectorDescriptor *sd, Voucher **v){
	/* if you are able to queue up sector descriptor immediately: */
	/* return a Voucher through *v and return 1 */
	/* otherwise, return 0 */
	//if we don't get a voucher right off the bat, don't bother doing anything
	if(nonblockingReadBB(voucher_b, (void**)v) == 0){
		return 0;
	}
	//otherwise we can do things with it!
	Voucher *p = *v;
	pthread_mutex_lock(&p->inUse);
	p->frw = 1;
	p->sd = sd;
	if(nonblockingWriteBB(read_b, p) == 1){
		p->finished = 0;
		*v = p;
		pthread_cond_signal(&p->usable);
		pthread_mutex_unlock(&p->inUse);
		return 1;
	}
	//if the nonblocking write failed, we need to return the voucher state and put it back in the voucher pool
	p->frw = 0;
	p->sd = NULL;
	blockingWriteBB(voucher_b, p);
	pthread_cond_signal(&p->usable);
	pthread_mutex_unlock(&p->inUse);
	*v = NULL;
	return 0;
}

/*
 * the following call is used to retrieve the status of the read or write
 * the return value is 1 if successful, 0 if not
 * the calling application is blocked until the read/write has completed
 * regardless of the status of a read, the associated SectorDescriptor is returned in *sd
 */
int redeem_voucher(Voucher *v, SectorDescriptor **sd){
	pthread_mutex_lock(&v->inUse);
	while(!v->finished){
		pthread_cond_wait(&v->usable, &v->inUse);
	}
	if(v->frw == 1){
		*sd = v->sd;
	}
	int succ = v->success;
	pthread_mutex_unlock(&v->inUse);
	if(succ){
		return 1;
	}
	return 0;
}