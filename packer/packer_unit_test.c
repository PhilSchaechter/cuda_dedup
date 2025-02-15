#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "store_const.h"
#include "store_struct.h"
#include "store.h"
#include "packer_struct.h"
#include "packer.h"
#include "is_logger.h"


#define mb_to_write 400

int main ( void ) {

is_init_logging();

zlog_info(logger, "Process startup");

store_store_obj *mystore = store_create_store("/dev/loop0");

packer_packer_obj *mypacker = packer_create_new_packer(mystore, 5, 10);

zlog_info(logger, "Packer object size is: %ld", sizeof(packer_packer_obj));
uint64_t bytes_to_write = mb_to_write * 1024 * 1024;
uint64_t bytes_written = 0;

srand(997);

zlog_info(logger, "Generating random data of %d MB", mb_to_write);
unsigned char hash[16] = "0000000000000000";
while (bytes_to_write > bytes_written) {
	uint32_t random = rand() % 100000 + 2000;
	bytes_written += random;
	unsigned char *newbuff = malloc(random);
	store_slice_flags_addressable flags;
	int add = packer_add_slice(mypacker, hash, 666, random, flags, newbuff);
	free(newbuff);
	//zlog_debug(logger, "Slice of length %u\t added to box %d", random, add);
	
	}

zlog_info(logger, "Finished writing, %u > %u", bytes_written, bytes_to_write);
packer_shutdown_packer(mypacker);
free(mystore);
zlog_info(logger, "Hello, world!");
return 0;
}

