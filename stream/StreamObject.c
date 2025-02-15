#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "rbuf.h"
#include "StreamObject.h"
#include <stdint.h>
#include <string.h>

#define KB 1024
#define BUFFER_BLOCK_SIZE 4*KB
#define BUFFER_SIZE 4*KB*KB

/*
 * StreamObject.c
 *
 *  Created on: Oct 30, 2018
 *      Author: Alex
 */

typedef enum
{
	FALSE,
	TRUE
} boolean;


struct stream_obj_struct
{
	char *head, *tail;
	char buffer[BUFFER_SIZE];
	uint16_t unit_counter;
	boolean is_reversed;
	boolean is_stream_started;
};


StreamObject CreateStreamObject()
{
	StreamObject stream_obj = (struct stream_obj_struct*)malloc(sizeof(struct stream_obj_struct));
	if (stream_obj == NULL)
	{
		printf("Couldn't allocate memory for new Stream object\n");
		return NULL;
	}

	stream_obj->head = stream_obj->buffer;
	stream_obj->tail = stream_obj->buffer;
	stream_obj->unit_counter = 0;
	stream_obj->is_reversed = FALSE;
	stream_obj->is_stream_started = FALSE;
	return stream_obj;
}

void FreeStreamObject(StreamObject stream_obj)
{
	if (stream_obj != NULL)
	{
		free(stream_obj);
	}
}

void StartStreamReading(StreamObject stream_obj)
{
	char *buffer_location=NULL;
	char ch[BUFFER_BLOCK_SIZE];
	char *res;
	char *anchor;
	boolean run_loop = TRUE;

	if (stream_obj == NULL)
	{
		printf("Got empty stream object\n");
		return;
	}

	printf("Marking stream as started\n");
	stream_obj->is_stream_started = TRUE;
	// read BUFFER_BLOCK_SIZE from stdin
	while(run_loop)
//	while(read(STDIN_FILENO, ch, BUFFER_BLOCK_SIZE) > 0)
	{
//		get BUFFER_BLOCK_SIZE chunk from stdin
		int read_size = read(STDIN_FILENO, ch, BUFFER_BLOCK_SIZE);
		if (read_size == 0){
			printf("No more reads, breaking...\n");
			break;
		}else{
			if (ch[read_size] == EOF){
				printf("EOF found\n");
				run_loop = FALSE;
			}
			printf("Got a new chunk!!!\n");
			anchor = get_anchor(stream_obj);	// check if has anchor
			if (!anchor)
				continue;

			hash_value = get_hash_value(stream_obj->buffer, anchor);	// get hash value

			if (is_new_hash(hash_value)){
				// compress and send to Packer
				chunk_size = anchor - stream_obj->buffer;
			}



			printf("Chunk size is: %d\n", read_size);
			printf("Last char -1 is: '%c'\n", ch[read_size-1]);
			printf("Last char is: '%c'\n", ch[read_size]);
			printf("Last char +1 is: '%c'\n", ch[read_size+1]);
			buffer_location = stream_obj->buffer + (stream_obj->unit_counter * BUFFER_BLOCK_SIZE);
			res = strcpy(buffer_location, ch);
			if (res != NULL)
			{
				printf("strcpy returned: %s\n", res);
			}else{
				printf("strcpy FAILED!!\n");
			}
			stream_obj->unit_counter += 1;

		}



		// check if has anchor

		//		stream_obj->head += BUFFER_BLOCK_SIZE;
		//		stream_obj->tail += BUFFER_BLOCK_SIZE;
	}


	stream_obj->is_stream_started = FALSE;
	printf("Marking stream as done\n");

}


// Code to check input size
//	int    c;
//	int s = 0;

//	while ((c = getchar()) != EOF)
//	{
//	  s++;
//	}
//
//	printf("Size: %d\n", s);
//	return;

