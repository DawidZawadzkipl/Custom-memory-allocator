#ifndef CUSTOM_MEMORY_ALLOCATOR_HEAP_H
#define CUSTOM_MEMORY_ALLOCATOR_HEAP_H
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

struct memory_manager_t
{
    void *memory_start;
    size_t memory_size;
    struct memory_chunk_t *first_memory_chunk;
    struct memory_chunk_t* head;
    struct memory_chunk_t* tail;
};

struct memory_chunk_t
{
    struct memory_chunk_t* prev;
    struct memory_chunk_t* next;
    size_t size;
    int free;
    uint32_t control_sum;
};

extern struct memory_manager_t memory_manager;

enum pointer_type_t
{
    pointer_null,
    pointer_heap_corrupted,
    pointer_control_block,
    pointer_inside_fences,
    pointer_inside_data_block,
    pointer_unallocated,
    pointer_valid
};

size_t size_align(size_t size);
struct memory_chunk_t* add_new_block(size_t size, struct memory_chunk_t* next_block);
size_t block_size(struct memory_chunk_t* ptr);
void coalesce_next(struct memory_chunk_t* curr);

int heap_setup(void);
void heap_clean(void);
void* heap_malloc(size_t size);
void* heap_calloc(size_t number, size_t size);
void* heap_realloc(void* memblock, size_t size);
void  heap_free(void* memblock);
int heap_validate(void);
enum pointer_type_t get_pointer_type(const void* const pointer);
size_t heap_get_largest_used_block_size(void);
int heap_expand(size_t size);
uint32_t calculate_control_sum(struct memory_chunk_t* chunk);
void set_control_sum(struct memory_chunk_t* chunk);

#endif //CUSTOM_MEMORY_ALLOCATOR_HEAP_H
