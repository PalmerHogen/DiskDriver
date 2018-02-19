#ifndef _FREE_SECTOR_DESCRIPTOR_STORE_FULL_H_
#define _FREE_SECTOR_DESCRIPTOR_STORE_FULL_H_

/*
 * This file is a component of the test harness and/or sample solution
 * to the DiskDriver exercise used in CIS415, Project 2, at the Univ of Oregon
 */

/*
 * extension to header file for FreeSectorDescriptorStore ADT to
 * define constructor and destructor
 */

#include "freesectordescriptorstore.h"

FreeSectorDescriptorStore *create_fsds(void);
void destroy_fsds(FreeSectorDescriptorStore *fsds);

#endif /* _FREE_SECTOR_DESCRIPTOR_STORE_FULL_H_ */
