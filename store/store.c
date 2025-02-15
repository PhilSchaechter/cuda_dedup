#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <assert.h>
#include <stdbool.h>
#include <pthread.h>
#include "store_const.h"
#include "store_struct.h"
#include "store.h"
#include "is_logger.h"


store_store_obj *store_create_store(char *device_name) {

	store_store_obj *new_store = malloc(sizeof(store_store_obj));
	strcpy(new_store->store_dev_name, device_name);

	pthread_mutex_init(&new_store->lock, NULL);

	if (store_open_store(new_store) >= 0) { zlog_info(logger, "Opened store"); } else { return NULL; }

	int current_tail_id = store_scan_box_headers (new_store);
	return new_store;
}

// opens a store for use
int store_open_store ( store_store_obj *store) {

store->store_dev_ptr = open(store->store_dev_name, O_RDWR);
store->store_head = 0;
store->store_tail = 0;

assert(store->store_dev_ptr >0);


return 1;
}

// scans the store to determine which boxes are allocated and which are not
int store_scan_box_headers ( store_store_obj *store) {

uint32_t at_box = 0;
uint16_t this_box_header;
bool found = false;

zlog_info(logger, "Scanning %u box headers", STORE_MAX_BOXES);
pthread_mutex_lock(&store->lock);
if ( lseek(store->store_dev_ptr, 0, SEEK_SET) < 0) {
	printf("Failed to inital seek store!\n");
	return -1;
	}

while (at_box < STORE_MAX_BOXES) {
	if ( read(store->store_dev_ptr, &this_box_header, sizeof(this_box_header)) != sizeof(this_box_header) ) {
		zlog_error(logger, "Failed to read store at offset %d!", (STORE_BOX_SIZE*at_box));
		pthread_mutex_unlock(&store->lock);
		return -1;
		}
	store->boxes[at_box] = this_box_header;	
	if (!found && this_box_header > 0) {
		store->store_head = at_box;
		zlog_info(logger, "HEAD of store found at box: %u with count: %u", at_box, this_box_header);
		found = true;
		}
	else if ( found && this_box_header == 0 ) {
		store->store_tail = at_box;
		zlog_info(logger, "TAIL of store found at box: %u with count: %u", at_box, this_box_header);
		pthread_mutex_unlock(&store->lock);
		return (store->store_tail); // returns the current tail
		}

	else if (found) { 
		zlog_debug(logger, "...found additional box at: %u with count: %u", at_box, this_box_header);
		}
	at_box++;
	if ( lseek(store->store_dev_ptr, STORE_BOX_SIZE*at_box, SEEK_SET) < 0 ) {
		pthread_mutex_unlock(&store->lock);
		zlog_error(logger, "Failed to seek store at offset %d!\n", (STORE_BOX_SIZE*at_box));
		return -1;
		}
	}

pthread_mutex_unlock(&store->lock);
return (store->store_tail); // returns the current tail
}



// allocates a new box from the tail of the store
store_box_in_memory *store_allocate_tail ( store_store_obj *store ) {

store_box_in_memory *new_box_in_memory;
uint32_t new_box_id;

if (store->store_tail < STORE_MAX_BOXES) {
	pthread_mutex_lock(&store->lock);
	new_box_id = store->store_tail;
	store->store_tail++;	
	pthread_mutex_unlock(&store->lock);

	new_box_in_memory = malloc(sizeof(store_box_in_memory));
	new_box_in_memory->box_id = new_box_id;
	new_box_in_memory->store_box_total_size = sizeof( new_box_in_memory->store_box_slice_count);  // box size starts with size of box header
	new_box_in_memory->store_box_slice_count = 0;
	new_box_in_memory->head = NULL;
	new_box_in_memory->tail = NULL;
	new_box_in_memory->next = NULL;
	new_box_in_memory->prev = NULL;
	pthread_spin_init(&new_box_in_memory->lock, 0);
	}
else { 
	// oos
	zlog_error(logger, "store out of space");
	return NULL;
	}

zlog_info(logger, "Box %d allocated from tail of store", new_box_id);
return new_box_in_memory;
}

// writes an entire box object to disk.  The packer should make up store_box_in_memory objects, which 
// when are determined to be full, will be sent here for storage.
int store_write_box_to_disk(store_store_obj *store, store_box_in_memory *box) {

store_slice_unstored *walker = box->head;
store_slice_header_disk *disk_slice = malloc(sizeof(store_slice_header_disk));
int at = 0; 
uint8_t *box_buf = malloc( sizeof( box->store_box_slice_count) + (sizeof(store_slice_header_disk)*box->store_box_slice_count) + box->store_box_total_size );
assert(sizeof(box_buf) < STORE_BOX_SIZE);

memcpy(box_buf, &box->store_box_slice_count, sizeof( box->store_box_slice_count));
at+=sizeof( box->store_box_slice_count);


while (walker != NULL) {
	memcpy(disk_slice->hash,walker->hash, HASH_SIZE);
	disk_slice->orig_size = walker->orig_size;
	disk_slice->deflate_size = walker->deflate_size;
	disk_slice->flags = walker->flags;
	
	memcpy(box_buf+at, disk_slice, sizeof(store_slice_header_disk));
	at += sizeof(store_slice_header_disk);
	walker = walker->next;
}
free(disk_slice);
walker = box->head;

while (walker != NULL) {
	memcpy(box_buf+at, walker->slice_data, walker->deflate_size);
	at += walker->deflate_size;
	walker = walker->next;
}
assert(at <= STORE_BOX_SIZE);


if ( pwrite(store->store_dev_ptr, box_buf, at, STORE_BOX_SIZE*box->box_id) != at) {  return -1; }

store->boxes[box->box_id] = box->store_box_slice_count;
zlog_info(logger, "Wrote %d bytes to box id %u (slices: %u full: %f)", at, box->box_id, box->store_box_slice_count, (double)at/STORE_BOX_SIZE);
free(box_buf);
return 1;
}

store_box_in_memory *store_load_box_headers_from_store ( store_store_obj *store, uint16_t box_id) {

	if (store->boxes[box_id] == 0) { return NULL; }

	store_box_in_memory *new_box = malloc(sizeof(store_box_in_memory));
	new_box->box_id = box_id;
	new_box->store_box_slice_count = store->boxes[box_id];
	new_box->store_box_total_size = sizeof( new_box->store_box_slice_count);  // box size starts with size of box header

	new_box->head = NULL;
	new_box->tail = NULL;
	new_box->next = NULL;
	new_box->prev = NULL;
	pthread_spin_init(&new_box->lock, 0);

	uint8_t *buf = malloc( sizeof(new_box->store_box_slice_count) + (sizeof(store_slice_header_disk) * new_box->store_box_slice_count));

	if ( pread(store->store_dev_ptr, buf, (sizeof(store_slice_header_disk) * new_box->store_box_slice_count), STORE_BOX_SIZE*new_box->box_id) != (sizeof(store_slice_header_disk) * new_box->store_box_slice_count)) {  return NULL; }

	assert( *(uint16_t *)buf == store->boxes[box_id]);

	// yikes
	int i = 0;
	for (i = 0; i < new_box->store_box_slice_count; i++) {
		store_slice_unstored *new_slice = malloc(sizeof(store_slice_unstored));
		memcpy(new_slice->hash, (*(store_slice_header_disk *)&buf[2+(i*sizeof(store_slice_header_disk))]).hash, HASH_SIZE);
		new_slice->orig_size = (*(store_slice_header_disk *)&buf[2+(i*sizeof(store_slice_header_disk))]).orig_size;
		new_slice->flags = (*(store_slice_header_disk *)&buf[2+(i*sizeof(store_slice_header_disk))]).flags;
		new_slice->deflate_size = (*(store_slice_header_disk *)&buf[2+(i*sizeof(store_slice_header_disk))]).deflate_size;
		new_slice->next = NULL;
		new_slice->slice_data = NULL;

		if (new_box->head == NULL) {
			new_box->head = new_slice;
			new_box->tail = new_slice;
		}
		else {
			new_box->tail->next = new_slice;
			new_box->tail = new_slice;
		}



	}


    free(buf);
	return new_box;
}

