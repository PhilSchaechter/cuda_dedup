#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include "store_struct.h"
#include "store_const.h"
#include "store.h"
#include "packer_const.h"
#include "packer_struct.h"
#include "packer.h"
#include "is_logger.h"

// creating a new packer should also create the thread which monitors that packer
packer_packer_obj *packer_create_new_packer ( store_store_obj *store,
		uint32_t packer_min_free_boxes,
		uint32_t packer_max_box_depth) {

	packer_packer_obj *mypacker = malloc(sizeof(packer_packer_obj));

	mypacker->head = NULL;
	mypacker->tail = NULL;
	mypacker->current_memory_boxes = 0;
	mypacker->max_memory_boxes = packer_max_box_depth;
	mypacker->min_free_memory_boxes = packer_min_free_boxes;
	mypacker->store = store;
	mypacker->monitor = 1;
	mypacker->box_write_when_larger_than =(int)(STORE_BOX_SIZE * PACKER_WRITE_BOX_WHEN_PERCENT_GREATER_THAN);
	pthread_mutex_init(&mypacker->lock, NULL);



	int i = 0;
	for (i = 0; i < packer_min_free_boxes; i++) {
		if ( packer_add_box_to_packer(mypacker) == NULL) {
			assert(1==0);
		}
	}

	if(pthread_create(&mypacker->packer_monitor_thread, NULL, packer_monitor, mypacker)) {
	    zlog_error(logger, "failed to create packer monitor thread");
	    return NULL;
	}


	// create packer monitor thread
return mypacker;
}

int packer_shutdown_packer(packer_packer_obj *packer) {

	zlog_info(logger, "Signaling packer monitor to shut down");
	packer->monitor = 0;
	while (packer->monitor != 2 ) {
		usleep(10);
	}
	pthread_join(packer->packer_monitor_thread, NULL);
	free(packer);
	return 1;
}

int packer_free_memory_box_and_all_slices(store_box_in_memory *box) {

	store_slice_unstored *walker = box->head;
	store_slice_unstored *prev = NULL;


	pthread_spin_destroy(&box->lock);
	while (walker != NULL) {
	    if (walker->slice_data != NULL) {
	    	free(walker->slice_data);
	    }
	    prev = walker;
	    walker = walker->next;
	    free(prev);
	}
	free(box);
	return 1;
}

int packer_add_slice (packer_packer_obj *packer,
		unsigned char *hash,
		uint32_t orig_size,
		uint32_t deflate_size,
		store_slice_flags_addressable flags,
		unsigned char *slice_data) {

// create the new NV slice object
store_slice_unstored *new_slice = malloc(sizeof(store_slice_unstored));

new_slice->deflate_size = deflate_size;
new_slice->orig_size = orig_size;
new_slice->flags = flags;
new_slice->slice_data = malloc(deflate_size); // should be a pointer we received with packer_add_slice 
new_slice->next = NULL;
memcpy(new_slice->slice_data, slice_data, deflate_size);  // the compressed slice from the compressor should be done in the nvdim!  Then we can avoid this memcpy
memcpy(new_slice->hash, hash, HASH_SIZE);

struct store_box_in_memory *walker = packer->head;

// Walk the packer box list
while (walker != NULL) {
	if ( (STORE_BOX_SIZE - walker->store_box_total_size) >= (sizeof(store_slice_header_disk) + deflate_size) ) {
		if ( pthread_spin_trylock(&walker->lock) == 0 ) {
			if ( packer_add_slice_to_box(walker, new_slice) >= 0 ) {
				// slice is stored in a box!
				pthread_spin_unlock(&walker->lock);
				return walker->box_id;  // returning here a uint and function is int...
			}
			else {
		        	pthread_spin_unlock(&walker->lock);
			}
		}

		// if we didn't add it, it might be a threading issue (box was full when we got there) try the next box.
	}

	walker = walker->next;
}
zlog_warn(logger, "No boxes to fit slice, inline packer adding new box");
// allocate new tail from store, add to packer box chain, then add slice to it, then return the box id
struct store_box_in_memory *new_box = packer_add_box_to_packer(packer);

int backoff = 50;
while (new_box == NULL) {
	usleep(backoff);
	new_box = packer_add_box_to_packer(packer);
	backoff = backoff * 2;
}

pthread_spin_lock(&new_box->lock);
if ( packer_add_slice_to_box(new_box, new_slice) >= 0 ) {
	pthread_spin_unlock(&new_box->lock);
	return(new_box->box_id);
}

zlog_error(logger, "No boxes to fit slice! ");
//PANIC
assert(1==0);
return -1;
}

static int packer_add_slice_to_box ( store_box_in_memory *box, store_slice_unstored *slice) {

	assert(box != NULL);
	assert(slice != NULL);

	if ((STORE_BOX_SIZE - box->store_box_total_size) < (sizeof(store_slice_header_disk) + slice->deflate_size)) {
		zlog_warn(logger, "Attempted to add slice to box that was too full");
		return -1;
	}

	if (box->head == NULL) {
		box->head = slice;
		box->tail = slice;
	}
	else {
		box->tail->next = slice;
		box->tail = slice;
	}
	box->store_box_slice_count++;
	box->store_box_total_size += (slice->deflate_size + sizeof(store_slice_header_disk));

    assert(box->store_box_total_size <= STORE_BOX_SIZE);

return box->box_id;
}


int packer_flush_boxes_to_disk ( packer_packer_obj *packer) {
	uint64_t flushed = 0;
	bool error = false;
	while (packer->head != NULL) {

		if ( packer_flush_head_to_disk(packer, true) != 1 ) {
			if (error) {
				zlog_error(logger, "Couldn't flush all boxes even after retry.  Eek");
				return flushed;

			}
			error = true;
		    zlog_warn(logger, "Couldn't flush head to disk in flush_all_boxes, retrying");
		    usleep(50);

		} else {
			error = false;
		    flushed++;
		}
	}

    return flushed;
}

int packer_flush_head_to_disk (packer_packer_obj *packer, bool packer_locked) {

struct store_box_in_memory *next = NULL;
struct store_box_in_memory *old_head = packer->head;

if (packer->head == NULL) {
	zlog_error(logger, "Packer attempted to flush null head to disk");
	return 0;
}
zlog_debug(logger, "Packer flushing current head, box %d to disk", packer->head->box_id);

// acquire needed locks
pthread_spin_lock(&packer->head->lock);
if (packer->head->next != NULL) {
   pthread_spin_lock(&packer->head->next->lock);
   next = packer->head->next;
} else if (packer_locked == false) {
	zlog_error(logger, "Cannot flush a single box to disk");
	pthread_spin_unlock(&packer->head->lock);
	return 0;
}

if ( store_write_box_to_disk(packer->store, packer->head) == 1 ) {
	pthread_mutex_lock(&packer->lock);
	// recheck head...could be someone added a box before we got here.
	if (next == NULL && packer->head->next != NULL) {
		zlog_warn(logger, "Head->next became non-null in the midst of packer work");
		pthread_spin_lock(&packer->head->next->lock);
		next = packer->head->next;
	}

	packer->head = next;
	packer->current_memory_boxes--;
	pthread_mutex_unlock(&packer->lock);

	if (next != NULL) {
		next->prev = NULL;
		pthread_spin_unlock(&next->lock);
	}

	pthread_spin_unlock(&old_head->lock);
	if ( packer_free_memory_box_and_all_slices(old_head) != 1 )  {
	    zlog_error(logger, "COULD NOT FREE NV MEMORY BOX");
	}

}
else {
	zlog_error(logger, "Packer was unable to flush head to disk, leaving head in place");
	pthread_spin_unlock(&packer->head->lock);
	if (next != NULL) {
		next->prev = NULL;
		pthread_spin_unlock(&next->lock);
	}
	return 0;
}


return 1;
}



/*
 * Creates new memory boxes (should be in nvdimm) for buffering client writes to disk
 * Can be called by a client if there is no space left...a nice mutex will slow some clients
 * down if we get overloaded.
 */
store_box_in_memory * packer_add_box_to_packer ( packer_packer_obj *packer ) {
	pthread_mutex_lock(&packer->lock);
	if (packer->current_memory_boxes >= packer->max_memory_boxes) {
		zlog_error(logger, "Packer is over box limit");
		pthread_mutex_unlock(&packer->lock);
		return NULL;
	}

	if (packer->tail == NULL && packer->head == NULL) {
		packer->tail = store_allocate_tail(packer->store);
		packer->head = packer->tail;
		assert(packer->tail != NULL);
	}
	else {
		packer->tail->next = store_allocate_tail(packer->store);
		assert(packer->tail->next != NULL);
		packer->tail->next->prev = packer->tail;
		packer->tail = packer->tail->next;
	}
	packer->current_memory_boxes++;
    pthread_mutex_unlock(&packer->lock);
return packer->tail;
}

/* packer manager thread
 *
 * give absolute priority to writing data to disk when it's ready.  While idle, tries to
 * make sure the buffer is the right size.   Relies on inline client adds to the packer
 * chain if the packer is busy writing data to disk.
 *
 *
 */
void * packer_monitor (packer_packer_obj *packer) {
	zlog_debug(logger, "Packer monitoring thread startup");
	struct store_box_in_memory *new_box;

	while (packer->monitor == 1) {
		usleep(10);
		if (packer->head == NULL) { continue; }
		while (packer->head->store_box_total_size >= packer->box_write_when_larger_than) {
			zlog_debug(logger, "Packer monitor: box %d size %d is greater than minimum flush size: %d", packer->head->box_id, packer->head->store_box_total_size, packer->box_write_when_larger_than);
			if ( packer_flush_head_to_disk(packer, false) != 1) {
				zlog_error(logger, "Packer monitor failed to flush head to disk.  Exiting");
				return NULL;
			}
		}
		while (packer->monitor == 1 && packer->current_memory_boxes < packer->min_free_memory_boxes) {
			new_box = packer_add_box_to_packer(packer);
			assert(new_box != NULL);
			zlog_info(logger, "Packer monitor: added new box to packer: %d", new_box->box_id);
		}

	}

	int count = packer_flush_boxes_to_disk(packer);
	zlog_info(logger, "Packer flushed %d boxes to disk", count);
	packer->monitor = 2;
	zlog_debug(logger, "Packer monitoring thread shutdown");

return NULL;
}

