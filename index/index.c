/*
 * index.c
 *
 *  Created on: Nov 6, 2018
 *      Author: phil
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include "index_const_dev.h"
#include "index.h"
#include "store.h"
#include "packer_const.h"
#include "packer_struct.h"
#include "packer.h"

index_index_obj *index_create_new_index() {
	index_index_obj *new_index = malloc(sizeof(index_index_obj));
	int i;
	for (i = 0; i < INDEX_NUM_HASH_BUCKETS; i++) {
		new_index->table[i] = NULL;
	}
	for (i = 0; i < INDEX_LOCKING_SEGMENTS; i++) {
		pthread_spin_init(&new_index->locks[i], 0);
	}

	// create cache expunger thread;
	return new_index;
}

uint64_t index_import_memory_box_to_index (index_index_obj *index,
		store_box_in_memory *box,
		bool headers_only) {

    uint64_t count = 0;
    store_slice_unstored *walker = box->head;
    while (walker != NULL) {
    	index_node *new_node = NULL;
    	if (headers_only) {
    		new_node = index_insert_node(index,
    			walker->hash,
    			NULL,
    			walker->orig_size,
    			100,
    			box->box_id,
    			walker->flags);
    	}
        if (new_node == NULL ) {
        	zlog_error(logger, "Failed to import hash %s from memory box %d to index", walker->hash, box->box_id);
        } else {
        	count++;
        }
        walker = walker->next;
    }
    if (headers_only) {
    	packer_free_memory_box_and_all_slices(box);
    }
    return count;
}

int index_generate_index_stats(index_index_obj *this_index) {

	uint64_t i;
	uint64_t hwm = 0;
	zlog_info(logger, "Checking %d locking segments of the index", INDEX_LOCKING_SEGMENTS);
	for (i = 0; i < INDEX_LOCKING_SEGMENTS; i++) {
		if ( pthread_spin_trylock(&this_index->locks[i]) == 0 ){
			pthread_spin_unlock(&this_index->locks[i]);
		}
		else {
		    zlog_warn(logger, "Lock range: %u is LOCKED", i);
		}
	}

	uint64_t allocated_buckets = 0;
	uint64_t allocated_nodes = 0;
	uint64_t this_depth = 0;
	zlog_info(logger, "Checking %d hash buckets of the index", INDEX_NUM_HASH_BUCKETS);
	for (i = 0; i < INDEX_NUM_HASH_BUCKETS; i++) {
		if (this_index->table[i] == NULL) { continue; }
		allocated_buckets++;
		allocated_nodes++;
		index_node *walker = this_index->table[i];
		this_depth = 1;
		while ( walker->flags.bits.leaf != 1 ) {
			allocated_nodes++;
			this_depth++;
		    walker = walker->next;
		}
		if (this_depth > hwm) { hwm = this_depth; }

	}
	zlog_info(logger, "%u allocated index buckets", allocated_buckets);
	zlog_info(logger, "%u allocated index nodes", allocated_nodes);
	zlog_info(logger, "%u is the longest hash chain", hwm);
	return 1;
}

uint32_t index_lookup_hash_for_writes(index_index_obj *this_index, unsigned char * hash) {

	//IFDEF DEV SYSTEM
	uint16_t *prefix = (uint16_t *)hash;
	//IFDEF PROD SYSTEM
	//uint32_t *prefix = (uint32_t *)hash;

	// If the prefix doesn't exist
	if (this_index->table[*prefix] == NULL) { return 0; }

	// Create remainder only if we need it.
	char *remainder = hash+sizeof(*prefix);

	if (memcmp(remainder, this_index->table[*prefix]->remainder, INDEX_NODE_HASH_LENGTH) == 0) {
		    uint16_t new_weight = rand() % INDEX_MAX_WEIGHT;
		    if (new_weight > this_index->table[*prefix]->weight) { this_index->table[*prefix]->weight = new_weight; }
			return this_index->table[*prefix]->orig_size;
	}

	index_node *walker = this_index->table[*prefix];

	while (!walker->flags.bits.leaf) {
		walker = walker->next;
		if (memcmp(remainder, walker->remainder, INDEX_NODE_HASH_LENGTH) == 0) {
		    uint16_t new_weight = rand() % INDEX_MAX_WEIGHT;
		    if (new_weight > walker->weight) { walker->weight = new_weight; }
			return walker->orig_size;
		}
	}

	return 0;

}

int index_release_lock_for_node(index_index_obj *this_index,
		unsigned char *hash,
		index_node *narrow_lock) {

	//IFDEF DEV SYSTEM
	//uint16_t *prefix = (uint16_t *)hash;
	uint8_t *lockfix = (uint8_t *)hash;
	//IFDEF PROD SYSTEM
	//uint32_t *prefix = (uint32_t *)hash;
	//uint16_t *lockfix = (uint16_t *)hash;


	if (narrow_lock == NULL) {
		pthread_spin_unlock(&this_index->locks[*lockfix]);
		return 1;
	} else {
	    pthread_spin_unlock(&narrow_lock->lock);
	    return 2;
	}
}

index_node *index_acquire_lock_for_node(index_index_obj *this_index,
		unsigned char *hash) {

	//IFDEF DEV SYSTEM
	uint16_t *prefix = (uint16_t *)hash;
	uint8_t *lockfix = (uint8_t *)hash;
	//IFDEF PROD SYSTEM
	//uint32_t *prefix = (uint32_t *)hash;
	//uint16_t *lockfix = (uint16_t *)hash;


    pthread_spin_lock(&this_index->locks[*lockfix]);
	// If the prefix doesn't exist
	if (this_index->table[*prefix] == NULL) { return NULL; }

    // attempt a narrow lock
	index_node *walker = this_index->table[*prefix];

		do {
			if (walker->flags.bits.inmemory == 1 ) {
				pthread_spin_lock(&walker->lock);
				pthread_spin_unlock(&this_index->locks[*lockfix]);
				return walker;
			}
			else if (walker->flags.bits.leaf != 1) { walker = walker->next; }
		} while (walker->flags.bits.leaf != 1);

		return NULL;

}

index_node *index_insert_node(index_index_obj *this_index,
		unsigned char *hash,
		unsigned char *slice_data,
		uint32_t orig_size,
		uint32_t weight,
		uint32_t box_id,
		store_slice_flags_addressable flags) {

	//IFDEF DEV SYSTEM
	uint16_t *prefix = (uint16_t *)hash;

	//IFDEF PROD SYSTEM
	//uint32_t *prefix = (uint32_t *)hash;


	char *remainder = hash+sizeof(*prefix);

	// decide here what kind of object we want to use (did we get hash data, or not?)
	// We can know if this will be a "loaded" object or not.

	//create node object
    index_node *new_node = malloc(sizeof(index_node));
    memcpy(new_node->remainder, remainder, INDEX_NODE_HASH_LENGTH);
    new_node->weight = weight;
    new_node->box_id = box_id;
    new_node->orig_size = orig_size;
    new_node->slice_data = slice_data;
    new_node->next = NULL;
    new_node->flags.byte = flags.byte;
    new_node->flags.bits.leaf = 1;
    pthread_spin_init(&new_node->lock, 0 );


    index_node *node_lock = index_acquire_lock_for_node(this_index, hash);
	// If the prefix doesn't exist
	if (this_index->table[*prefix] == NULL) {
		this_index->table[*prefix] = new_node;
		index_release_lock_for_node(this_index, hash, node_lock);
		return new_node;
	}

	index_node *walker = this_index->table[*prefix];
	index_node *prev = NULL;


	while (!walker->flags.bits.leaf) {
		if (memcmp(remainder, walker->remainder, INDEX_NODE_HASH_LENGTH) == 0) {
			index_release_lock_for_node(this_index, hash, node_lock);
			// need to release the new node here, we don't need it any more.
			zlog_warn(logger, "Duplicate insert attempted for index bucket %u", *prefix);
			return walker;
		}

		prev = walker;
		walker = walker->next;

		}

	if (walker->flags.bits.inmemory) {
		// simples, just update him.
		walker->flags.bits.leaf = 0;
		walker->next = new_node;
		index_release_lock_for_node(this_index, hash, node_lock);
	}
	else {
	    // hard, convert walker to branch type, need to update prev as well.
	    walker->flags.bits.leaf = 0;
	    walker->next = new_node;
	    // update prev here after next is pointing
	    // can now unlock spinlock
	    index_release_lock_for_node(this_index, hash, node_lock);
	    // set "old" node values to null/zero
	    // free old node
	}

	return new_node;
}
