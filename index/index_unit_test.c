/*
 * index_unit_test.c
 *
 *  Created on: Nov 7, 2018
 *      Author: phil
 */


#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include "is_logger.h"
#include "index.h"

#define generate_nodes 1250000

int main ( void ) {

is_init_logging();
srand(997);

char *fake_hash = malloc(16);
char *fake_hash2 = malloc(16);
char *fake_hash3 = malloc(16);

strcpy(fake_hash, "123456789012345");
strcpy(fake_hash2, "123456781012345");
strcpy(fake_hash3, "123456789012349");

index_index_obj *myindex = index_create_new_index();
store_slice_flags_addressable flags;

flags.byte = 0;

// Check for new node in empty index - should be null
uint32_t test = index_lookup_hash_for_writes(myindex, fake_hash);
assert(test == 0);

zlog_debug(logger, "Index obj size: %zu, Spinlock size is %zu, loaded node size is %zu, unloaded branch: %zu, opaque: %zu", sizeof(index_index_obj), sizeof(myindex->locks[0]), sizeof(index_node), sizeof(index_node_unloaded_branch), sizeof(index_node_opaque));
// insert node into index - should return node
index_node *test_node = index_insert_node(myindex, fake_hash, NULL, 666, 100, 1, flags);
assert(test_node != NULL);
test_node = index_insert_node(myindex, fake_hash2, NULL, 777, 100, 1, flags);
assert(test_node != NULL);
test_node = index_insert_node(myindex, fake_hash3, NULL, 888, 100, 1, flags);
assert(test_node != NULL);
zlog_info(logger, "Weight of new node is: %u", test_node->weight);

// Check for new node in index post insert - should return node.
int i;
zlog_debug(logger, "Conducting 100000 lookups");
clock_t begin = clock();
for (i = 0; i<100000; i++) {
    test = index_lookup_hash_for_writes(myindex, fake_hash3);
    assert(test == 888);
}
clock_t end = clock();
double time_spent = (double)(end - begin)/CLOCKS_PER_SEC;
zlog_info(logger, "Lookups took %f seconds", time_spent);
zlog_info(logger, "Weight of new node is: %u", test_node->weight);

int j;
zlog_info(logger, "Generating %d random hashes", generate_nodes);
double accum = 0;
for (i = 0; i<generate_nodes; i++) {
    for (j = 0; j<16; j++) {
    	fake_hash[j] = rand() % 255;
    }
    

    clock_t begin = clock();
    test_node = index_insert_node(myindex, fake_hash, NULL, 666, 100, 1, flags);
    clock_t end = clock();
    accum += (double)(end - begin);

    assert (test_node != NULL);
    //zlog_debug(logger, "Generated node %d", i);
    if (pthread_spin_trylock(&test_node->lock) != 0) {
    	zlog_error(logger, "Received locked node! Shouldn't be possible");
    	return 0;
    } else { pthread_spin_unlock(&test_node->lock); }
}
zlog_info(logger, "Hash inserts took %f seconds", accum/CLOCKS_PER_SEC);
index_generate_index_stats(myindex);



zlog_info(logger, "Hello, World!");

}
