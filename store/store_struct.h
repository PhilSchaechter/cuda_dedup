#ifndef STORE_STRUCT_H
#define STORE_STRUCT_H

#include "store_const.h"


#pragma pack(push, 1)
struct store_slice_flags;
typedef struct store_slice_flags {
	unsigned int compression:2;
	unsigned int deleted:1; // do we need this?  Maybe if it's deleted it's just gone.
	unsigned int ondisk:1;
	unsigned int oncloud:1;
	unsigned int inmemory:1; // used to mark if the node is a "loaded" node or not
	unsigned int leaf:1; // used to mark if the node is a leaf node or not
	unsigned int recompressed:1; // if the slice has undergone recompression
} store_slice_flags;

union store_slice_flags_addressable;
typedef union store_slice_flags_addressable {
	uint8_t byte;
	store_slice_flags bits;
} store_slice_flags_addressable;

// This slice holder is only for use internally by the store, it's for holding unstored
// slices before they are packed.  They are then also put into the index.
struct store_slice_unstored;
typedef struct store_slice_unstored {
	unsigned char hash[HASH_SIZE];
	uint32_t orig_size;
	uint32_t deflate_size;
	store_slice_flags_addressable flags;
	char *slice_data;
	struct store_slice_unstored *next;

} store_slice_unstored;


// This is the way the slice headers look on disk
struct store_slice_header_disk;
typedef struct store_slice_header_disk {
	unsigned char hash[HASH_SIZE];
	uint32_t orig_size;
	uint32_t deflate_size;
	store_slice_flags_addressable flags;
} store_slice_header_disk;
#pragma pack(pop)

// Object which holds a single store
struct store_store_obj;
typedef struct store_store_obj {
	char store_dev_name[64];
	int store_dev_ptr;
	uint32_t boxes[STORE_MAX_BOXES];
	uint32_t store_head;
	uint32_t store_tail;
	pthread_mutex_t lock;
} store_store_obj;

// Object which holds a box in memory (probably should be in NVDIMM)
struct store_box_in_memory;
typedef struct store_box_in_memory {
	uint32_t box_id;
	uint32_t store_box_total_size;
	uint16_t store_box_slice_count;
	store_slice_unstored *head;
	store_slice_unstored *tail;
	struct store_box_in_memory *prev;
	struct store_box_in_memory *next;
	pthread_spinlock_t lock;
} store_box_in_memory;

#endif
