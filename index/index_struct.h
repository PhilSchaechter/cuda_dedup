/*
 * index_struct.h
 *
 *  Created on: Nov 6, 2018
 *      Author: phil
 */

#ifndef INDEX_STRUCT_H
#define INDEX_STRUCT_H

#include <pthread.h>
#include "index_const_dev.h"
#include "store_struct.h"

#pragma pack(push, 1)

struct index_node_opaque;
typedef struct index_node_opaque {
	store_slice_flags_addressable flags;
	unsigned char remainder[INDEX_NODE_HASH_LENGTH]; // will be length -X of index size
	uint32_t box_id;
	uint16_t weight;
	uint32_t orig_size; // probably should be 24-bit
} index_node_opaque;


struct index_node_unloaded_branch;
typedef struct index_node_unloaded_branch {
	store_slice_flags_addressable flags;
	unsigned char remainder[INDEX_NODE_HASH_LENGTH]; // will be length -X of index size
	uint32_t box_id;
	uint16_t weight;
	uint32_t orig_size; // probably should be 24-bit
	struct index_node *next; // pointer to next hash in list (should be sorted on the way down)

} index_node_unloaded_branch;

struct index_node;
typedef struct index_node {
	store_slice_flags_addressable flags;
	unsigned char remainder[INDEX_NODE_HASH_LENGTH]; // will be length -X of index size
	uint32_t box_id;
	uint16_t weight;
	uint32_t orig_size; // probably should be 24-bit

	uint32_t viewers; // number of streams actively using this data
	struct index_node *next; // pointer to next hash in list (should be sorted on the way down)
	char *slice_data; // pointer to compressed slice data
	pthread_spinlock_t lock;

} index_node;
#pragma pack(pop)

struct index_index_obj;
typedef struct index_index_obj {
	struct index_node *table[INDEX_NUM_HASH_BUCKETS];
	pthread_spinlock_t locks[INDEX_LOCKING_SEGMENTS];
	uint64_t data_in_cache;
} index_index_obj;


#endif /* INDEX_STRUCT_H_ */
