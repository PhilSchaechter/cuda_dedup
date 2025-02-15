/*
 * system_struct.h
 *
 *  Created on: Nov 23, 2018
 *      Author: phil
 */

#ifndef SYSTEM_STRUCT_H_
#define SYSTEM_STRUCT_H_

#include "store_struct.h"
#include "store_const.h"
#include "store.h"
#include "packer_const.h"
#include "packer_struct.h"
#include "packer.h"
#include "index_const_dev.h"
#include "index_struct.h"
#include "index.h"

struct system_system_obj;
typedef struct system_system_obj {
    store_store_obj *store;
    packer_packer_obj *packer;
    index_index_obj *index;
} system_system_obj;

#endif /* SYSTEM_STRUCT_H_ */
