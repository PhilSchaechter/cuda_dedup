/*
 * system.c
 *
 *  Created on: Nov 23, 2018
 *      Author: phil
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include "is_logger.h"
#include "system.h"
#include "store.h"


int system_import_all_boxes_to_index(system_system_obj *system) {
	int at_box = 0;


	for (at_box = system->store->store_head; at_box<system->store->store_tail; at_box++) {
		store_box_in_memory *this_box = store_load_box_headers_from_store(system->store, at_box);
		if (this_box == NULL) {
			zlog_warn(logger, "Empty box found during import: %d", at_box);
			continue;
		}
		int import = index_import_memory_box_to_index(system->index, this_box, true);
		zlog_debug(logger, "Imported %d slice headers to index from box %d", import, at_box);
	}
	return at_box;
}

system_system_obj *system_create_new_system(char *device,
		char *device_file,
		uint64_t device_size_mega_bytes,
		bool create_device,
		bool format_device,
		bool load_index_from_disk) {

is_init_logging();
zlog_info(logger, "System startup");

char my_device_file[128];

// check parameters
if (strlen(device) <= 0) {
	zlog_error(logger, "Must specify a loop device, eg: /dev/loop7");
	return NULL;
}

if (create_device && geteuid() != 0) {
	zlog_error(logger, "Requested to add device, must be root.");
	return NULL;
}
else if (create_device || format_device) {
	char *username = getenv("USER");
	if (strlen(device_file) > 0 ) {
		strcpy(my_device_file, device_file);
		zlog_info(logger, "Using device file: %s", my_device_file);
	}
	else {

	    sprintf(my_device_file, "/tmp/loopbackfile.%s.img", username);
	    zlog_warn(logger, "Generated device file: %s", my_device_file);
	}
}
if (format_device) {
    char dd_string[128];
    sprintf(dd_string, "dd if=/dev/zero of=%s bs=1M count=%lu", my_device_file, device_size_mega_bytes);
    zlog_info(logger, "Executing dd: %s", dd_string);
    if ( system(dd_string) != 0 ) {
    	zlog_error(logger, "Unable to format device file");
    	return NULL;
    }
}

if (create_device) {
    char command_str[128];
    zlog_info(logger, "Removing device %s", device);
    sprintf(command_str, "losetup -d %s", device);
    system(command_str);

    sprintf(command_str, "losetup -P %s %s", device, my_device_file);
    zlog_info(logger, "Executing: %s", command_str);
    if ( system(command_str) != 0 ) {
    	zlog_error(logger, "Failed to create loop device");
    	return NULL;
    }

}




system_system_obj *mysystem = malloc(sizeof(system_system_obj));

// STORE
mysystem->store = store_create_store(device);
assert(mysystem->store != NULL);

// PACKER
mysystem->packer = packer_create_new_packer(mysystem->store, 5, 15);
assert(mysystem->packer != NULL);

// INDEX
mysystem->index = index_create_new_index();
assert(mysystem->index != NULL);

if (load_index_from_disk) {
	zlog_info(logger, "Importing box headers to index as requested");
	system_import_all_boxes_to_index(mysystem);
}

zlog_info(logger, "System creation complete");
return mysystem;

}

int system_start_system(system_system_obj *system) {


return 0;
}

