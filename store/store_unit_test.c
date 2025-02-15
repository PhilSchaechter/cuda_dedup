#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include "store_const.h"
#include "store_struct.h"
#include "store.h"
#include "is_logger.h"

int main ( void ) {

is_init_logging();
zlog_info(logger, "Process startup");

uint32_t current_tail_id = 0;

store_store_obj *mystore = malloc(sizeof(store_store_obj));
pthread_mutex_init(&mystore->lock, NULL);
strcpy(mystore->store_dev_name, "/dev/loop0");

if (store_open_store(mystore) >= 0) { zlog_info(logger, "Opened store"); } else { return 1; }

current_tail_id = store_scan_box_headers (mystore);
if (current_tail_id >= 0) { zlog_info(logger, "Scanned box headers of store, tail is %u", current_tail_id); } else { return 1; }

store_slice_unstored *new = NULL;
int i;
int j;
int random;
int slice_max;

srand(997);

for (i = 0; i < 10; i++) {
	new = NULL;
	store_box_in_memory *new_box = store_allocate_tail(mystore);

	assert(new_box->box_id == current_tail_id);  // check that the current tail id is the same as that allocated for the new box
	current_tail_id++;

	if (new_box != NULL ) { zlog_info(logger, "Allocated new box %u", new_box->box_id); } else { return 1; }
	new_box->store_box_slice_count = 0;
	new_box->store_box_total_size = 0;

	slice_max = rand() % 20;
	for (j = 0; j < slice_max; j++) {
		new = malloc(sizeof(store_slice_unstored));
		new->next = NULL;
		strcpy(new->hash, "00000000000000");
		random = rand() % 10000 + 8000;
		zlog_debug(logger, "Allocating new slice of length %d", random);
		new->slice_data = malloc(random);
		new->deflate_size = random;
		new->orig_size = 666;
		if (new_box->head == NULL) { new_box->head = new; new_box->tail = new; }
		else {
			new_box->tail->next = new;
			new_box->tail = new;
			}
		new_box->store_box_slice_count++;
		new_box->store_box_total_size += random;
		}

	if ( store_write_box_to_disk (mystore, new_box) >= 0) { zlog_info(logger, "Wrote %u slices in box %u to disk", new_box->store_box_slice_count, new_box->box_id); } else { return 1; }
	free(new_box);
	
	}


zlog_info(logger, "Loading first box written");
store_box_in_memory *loaded_box = store_load_box_headers_from_store(mystore, 0);
zlog_info(logger, "Checking first box first slice hash: %s", loaded_box->head->hash);
assert(loaded_box->head->orig_size == 666);
zlog_info(logger, "Hello, world!");
return 0;
}

