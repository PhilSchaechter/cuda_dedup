#ifndef STORE_H
#define STORE_H

#include "store_const.h"
#include "store_struct.h"

extern store_store_obj *store_create_store(char *device_name);
extern int store_open_store ( store_store_obj *store);
extern int store_format_store ( store_store_obj *store);


extern int store_save_box_to_store ( store_store_obj *store, store_box_in_memory *store_this_box);

extern store_box_in_memory *store_allocate_tail ( store_store_obj *store );
extern int store_write_box_to_disk(store_store_obj *store, store_box_in_memory *box);

// read
extern int store_scan_box_headers ( store_store_obj *store);
extern store_box_in_memory *store_load_box_from_store ( store_store_obj *store, uint16_t box_id);
extern store_box_in_memory *store_load_box_headers_from_store ( store_store_obj *store, uint16_t box_id);

#endif
