/*
 * index.h
 *
 *  Created on: Nov 6, 2018
 *      Author: phil
 */

#ifndef INDEX_H
#define INDEX_H

#include <pthread.h>
#include <stdbool.h>
#include "is_logger.h"
#include "index_const_dev.h"
#include "index_struct.h"
#include "store.h"

extern uint64_t index_import_memory_box_to_index (index_index_obj *index,
		store_box_in_memory *box,
		bool headers_only);

extern uint32_t index_lookup_hash_for_writes(index_index_obj *this_index, unsigned char * hash);

extern index_index_obj *index_create_new_index();

extern int index_add_node_to_index(index_index_obj *this_index, index_node *node);

extern index_node *index_insert_node(index_index_obj *this_index,
		unsigned char *hash,
		unsigned char *slice_data,
		uint32_t orig_size,
		uint32_t weight,
		uint32_t box_id,
		store_slice_flags_addressable flags);

extern index_node *index_acquire_lock_for_node(index_index_obj *this_index,
		unsigned char *hash);

extern int index_release_lock_for_node(index_index_obj *this_index,
		unsigned char *hash,
		index_node *narrow_lock);

extern int index_generate_index_stats(index_index_obj *this_index);



#endif /* INDEX_H_ */
