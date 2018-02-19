#ifndef _SECTOR_DESCRIPTOR_CREATOR_HDR_
#define _SECTOR_DESCRIPTOR_CREATOR_HDR_

/*
 * This file is a component of the test harness and/or sample solution
 * to the DiskDriver exercise used in CIS415, Project 2, at the Univ of Oregon
 */

/*
 * define the function prototype for create_free_sector_descriptors()
 */

#include "freesectordescriptorstore.h"

void create_free_sector_descriptors( FreeSectorDescriptorStore *fsds,
		                     void *mem_start, unsigned long mem_length);

#endif /* _SECTOR_DESCRIPTOR_CREATOR_HDR_ */
