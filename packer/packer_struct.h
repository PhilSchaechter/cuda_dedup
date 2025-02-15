#ifndef PACKER_STRUCT_H
#define PACKER_STRUCT_H


// Object which holds a box in memory (probably should be in NVDIMM)
struct packer_packer_obj;
typedef struct packer_packer_obj {
        uint32_t max_memory_boxes;
        uint32_t current_memory_boxes;
        struct store_box_in_memory *head;
        uint32_t min_free_memory_boxes;
        struct store_box_in_memory *tail;
	struct store_store_obj *store;
        pthread_mutex_t lock;
        uint32_t box_write_when_larger_than;
        pthread_t packer_monitor_thread;
        uint32_t monitor; // toggles off the packer monitor if needed.
} packer_packer_obj;

#endif
