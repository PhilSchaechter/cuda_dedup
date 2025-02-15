/*
 * system_unit_test.c
 *
 *  Created on: Nov 25, 2018
 *      Author: phil
 */




#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <stdbool.h>
#include "is_logger.h"
#include "system.h"
#include "store_struct.h"

int main (void) {

	system_system_obj *mysystem = system_create_new_system("/dev/loop0",
			"",
			1000,
			false,
			true,
			false);

assert(mysystem != NULL);

int i,j;
int generate_nodes = 10000;
char fake_hash[HASH_SIZE];
char first_hash[HASH_SIZE];
bool picked_random = false;
unsigned char *random_buff = NULL;
char random_hash[HASH_SIZE];


zlog_info(logger, "Generating %d random hashes", generate_nodes);
for (i = 0; i<generate_nodes; i++) {
    for (j = 0; j<16; j++) {
    	fake_hash[j] = rand() % 255;
    }
	uint32_t random = rand() % 100000 + 2000;
	unsigned char *newbuff = malloc(random);
	if (!picked_random && random % 12 == 0) {
		picked_random = true;
		strcpy(random_hash, fake_hash);
		random_buff = malloc(random);
		memcpy(random_buff, newbuff, random);
		zlog_info(logger, "Selected random hash for test: length is %d", random);
	}
	store_slice_flags_addressable flags;
	int add = packer_add_slice(mysystem->packer, fake_hash, random, random, flags, newbuff);
	//index_node *this_node = index_insert_node(mysystem->index, fake_hash, newbuff, random, 100, add, flags);
	//assert(this_node != NULL);
	free(newbuff);

}
zlog_info(logger, "Telling packer to shut down");
packer_shutdown_packer(mysystem->packer);
int import = system_import_all_boxes_to_index(mysystem);
zlog_info(logger, "Imported %d boxes to index", import);
uint32_t test = index_lookup_hash_for_writes(mysystem->index, fake_hash);
assert(test > 0);
zlog_info(logger, "Test slice had length %d", test);



index_generate_index_stats(mysystem->index);


}
