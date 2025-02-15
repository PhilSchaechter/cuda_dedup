/*
 * system.h
 *
 *  Created on: Nov 23, 2018
 *      Author: phil
 */

#ifndef SYSTEM_H_
#define SYSTEM_H_

#include <stdbool.h>

#include "system_struct.h"

extern system_system_obj *system_create_new_system(char *device,
		char *device_file,
		uint64_t device_size_mega_bytes,
		bool create_device,
		bool format_device,
		bool load_index_from_disk);

extern int system_start_system(system_system_obj *system);

extern int system_import_all_boxes_to_index(system_system_obj *system);

#endif /* SYSTEM_H_ */
