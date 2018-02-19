#ifndef _FREE_SECTOR_DESCRIPTOR_STORE_H_
#define _FREE_SECTOR_DESCRIPTOR_STORE_H_

/*
 * This file is a component of the test harness and/or sample solution
 * to the DiskDriver exercise used in CIS415, Project 2, at the Univ of Oregon
 */

/*
 * header file for FreeSectorDescriptorStore ADT
 */

#include "sectordescriptor.h"

typedef struct freesectordescriptorstore FreeSectorDescriptorStore;

/*
 * as usual, the blocking versions only return when they succeed
 * the nonblocking versions return 1 if successful, 0 otherwise
 * the _get_ functions set their final argument if they succeed
 */

void blocking_get_sd(FreeSectorDescriptorStore *fsds, SectorDescriptor **sd);
int nonblocking_get_sd(FreeSectorDescriptorStore *fsds, SectorDescriptor **sd);

void blocking_put_sd(FreeSectorDescriptorStore *fsds, SectorDescriptor *sd);
int nonblocking_put_sd(FreeSectorDescriptorStore *fsds, SectorDescriptor *sd);

#endif /* _FREE_SECTOR_DESCRIPTOR_STORE_H_ */
