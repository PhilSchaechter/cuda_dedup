#ifndef PACKER_H
#define PACKER_H

#include <stdbool.h>
#include "packer_const.h"

extern packer_packer_obj *packer_create_new_packer ( store_store_obj *store,
		uint32_t packer_min_free_boxes,
		uint32_t packer_max_box_depth);

extern int packer_add_slice (packer_packer_obj *packer,
		unsigned char *hash,
		uint32_t orig_size,
		uint32_t deflate_size,
		store_slice_flags_addressable flags,
		unsigned char *slice_data);

extern int packer_flush_boxes_to_disk ( packer_packer_obj *packer);

extern int packer_shutdown_packer(packer_packer_obj *packer);

static int packer_add_slice_to_box ( store_box_in_memory *box, store_slice_unstored *slice);

store_box_in_memory * packer_add_box_to_packer ( packer_packer_obj *packer );

extern int packer_free_memory_box_and_all_slices(store_box_in_memory *box);

static int packer_flush_head_to_disk (packer_packer_obj *packer, bool packer_locked);

static void *packer_monitor (packer_packer_obj *packer);

#endif
