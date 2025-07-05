#include "heap.h"

#define PAGE_SIZE 4096
#define ALIGN_SIZE 4
#define FENCE_SIZE 4
#define FENCE_VALUE 0xFA


struct memory_manager_t memory_manager;

uint32_t calculate_control_sum(struct memory_chunk_t* chunk){
    if(chunk==NULL) return 0;
    uint32_t control_sum = 0;
    if(chunk->prev!=NULL)
        control_sum += (uint32_t)(uintptr_t)chunk->prev;
    if(chunk->next!=NULL)
        control_sum += (uint32_t)(uintptr_t)chunk->next;
    control_sum += (uint32_t)chunk->size;
    control_sum += (uint32_t)chunk->free;
    return control_sum;
}

void set_control_sum(struct memory_chunk_t* chunk){
    if(!chunk) return ;
    chunk->control_sum = calculate_control_sum(chunk);
    if(chunk->prev != NULL){
        chunk->prev->control_sum = calculate_control_sum(chunk->prev);
    }
    if(chunk->next != NULL){
        chunk->next->control_sum = calculate_control_sum(chunk->next);
    }
}

size_t size_align(size_t size){
    return (size+ALIGN_SIZE-1)&~(ALIGN_SIZE-1);
}

void set_fence(void* ptr){
    for (int i = 0; i < FENCE_SIZE; ++i) {
        *((uint8_t*)ptr+i) = FENCE_VALUE;
    }
}

int check_fence(const uint8_t* ptr){
    for (int i = 0; i < FENCE_SIZE; ++i) {
        if(*(ptr+i)!=FENCE_VALUE)
            return 1;
    }
    return 0;
}

int heap_expand(size_t size) {
    struct memory_chunk_t* old_tail = memory_manager.tail;

    size_t page_multiplier = size/PAGE_SIZE + 1;
    void* mem_request = sbrk((intptr_t)page_multiplier * PAGE_SIZE);
    if(mem_request == (void*)-1){
        //przydzielenie potrzebnej pameici nieudane
        return -1;
    }
    memory_manager.memory_size += (page_multiplier * PAGE_SIZE);
    memory_manager.tail = (struct memory_chunk_t*)((uint8_t*)memory_manager.memory_start + memory_manager.memory_size - sizeof(struct memory_chunk_t));
    memory_manager.tail->free = 0;
    memory_manager.tail->size = 0;
    memory_manager.tail->next = NULL;
    memory_manager.tail->prev = old_tail->prev;
    memory_manager.tail->control_sum = 0;
    if(memory_manager.first_memory_chunk!=NULL){
        old_tail->prev->next = memory_manager.tail;
    }
    set_control_sum(memory_manager.tail);
    return 0;
}

struct memory_chunk_t* add_new_block(size_t size, struct memory_chunk_t* next_block){
    uint8_t* new_chunk_start = (uint8_t*)next_block->prev + block_size(next_block->prev);
    if ((new_chunk_start + block_size(memory_manager.head) + size_align(size)) >= (uint8_t*)memory_manager.memory_start + memory_manager.memory_size - sizeof(struct memory_chunk_t)) {
        return NULL;
    }
    struct memory_chunk_t* new_chunk = (struct memory_chunk_t*)new_chunk_start;
    new_chunk->size = size;
    new_chunk->free = 0;
    new_chunk->prev = next_block->prev;
    new_chunk->next = next_block;
    next_block->prev->next = new_chunk;
    next_block->prev = new_chunk;
    set_fence(new_chunk_start + sizeof(struct memory_chunk_t));
    set_fence(new_chunk_start + sizeof(struct memory_chunk_t) + FENCE_SIZE + size);
    set_control_sum(new_chunk);
    return new_chunk;
}

size_t block_size(struct memory_chunk_t* ptr){
    return (size_align(ptr->size) + sizeof(struct memory_chunk_t) + 2*FENCE_SIZE);
}

void coalesce_next(struct memory_chunk_t* curr){
    struct memory_chunk_t* next_chunk = curr->next;

//    curr->size+= block_size(next_chunk);
    curr->size = block_size(curr) + block_size(next_chunk) - sizeof(struct memory_chunk_t) - 2 * FENCE_SIZE;
    curr->size = size_align(curr->size);
    curr->next = next_chunk->next;
    next_chunk->next->prev = curr;
    set_control_sum(curr);
}

void split_block(struct memory_chunk_t* current){
    size_t remaining_size = (size_t) ((uint8_t *) current->next - ((uint8_t *) current + block_size(current)));
    if (remaining_size >= block_size(memory_manager.head)) {
        size_t new_block_size = remaining_size - block_size(memory_manager.head);
        struct memory_chunk_t *new_block = add_new_block(new_block_size, current->next);
        //new_block moze byc null jak nie mam miejsca!!
        if(new_block) {
            new_block->free = 1;
            set_control_sum(new_block);
        }
    }
}

int heap_setup(void){
    // [HF.....T]
    memory_manager.memory_start = sbrk(0);
    if(memory_manager.memory_start == (void *)-1){
        return -1;
    }
    //ALOKACJA 4096 BAJTOW DLA STERTY
    void* mem_request = sbrk(PAGE_SIZE);
    if(mem_request == (void*)-1){
        memory_manager.memory_start = NULL;
        return -1;
    }
    else{
        memory_manager.first_memory_chunk = NULL;
        //USTAWIENIE HEAD
        memory_manager.memory_size = PAGE_SIZE;
        memory_manager.head = (struct memory_chunk_t*)memory_manager.memory_start;
        memory_manager.head->size = 0;
        memory_manager.head->free = 0;
        memory_manager.head->prev = NULL;
        memory_manager.head->next = NULL;
        set_control_sum(memory_manager.head);
        //USTAWIENIE TAIL
        memory_manager.tail = (struct memory_chunk_t*)((uint8_t*)memory_manager.memory_start+memory_manager.memory_size-sizeof(struct memory_chunk_t));
        memory_manager.tail->size = 0;
        memory_manager.tail->free = 0;
        memory_manager.tail->next = NULL;
        memory_manager.tail->prev = NULL;
        set_control_sum(memory_manager.tail);
        return 0;
    }
}

void heap_clean(void){
    if(memory_manager.memory_start == NULL) return;
    void*curr_break = sbrk(0);
    intptr_t allocated = (intptr_t)((uint8_t*)curr_break - (uint8_t*)memory_manager.memory_start);
    sbrk(-allocated);
    memory_manager.memory_start = NULL;
    memory_manager.tail = NULL;
    memory_manager.head = NULL;
    memory_manager.first_memory_chunk = NULL;
    memory_manager.memory_size = 0;
}

void* heap_malloc(size_t size){
    if(heap_validate()!=0 || size<1 ) return NULL;

    //pierwszy malloc
    if(memory_manager.first_memory_chunk == NULL){
        if(size>memory_manager.memory_size-3*sizeof(struct memory_chunk_t) - 2 * FENCE_SIZE){
            if(heap_expand(size) == -1){
                return NULL;
            }
        }
        memory_manager.first_memory_chunk = (struct memory_chunk_t*)((uint8_t*)memory_manager.memory_start+sizeof(struct memory_chunk_t));
        memory_manager.first_memory_chunk->prev = memory_manager.head;
        memory_manager.first_memory_chunk->next = memory_manager.tail;
        memory_manager.head->next = memory_manager.first_memory_chunk;
        memory_manager.tail->prev = memory_manager.first_memory_chunk;
        memory_manager.first_memory_chunk->size = size;
        memory_manager.first_memory_chunk->free = 0;
        set_fence((void*)((uint8_t*)memory_manager.first_memory_chunk+sizeof(struct memory_chunk_t)));
        set_fence((void*)((uint8_t*)memory_manager.first_memory_chunk+sizeof(struct memory_chunk_t)+memory_manager.first_memory_chunk->size+FENCE_SIZE));
        set_control_sum(memory_manager.first_memory_chunk);
        return (void*)((uint8_t*)memory_manager.first_memory_chunk+sizeof(struct memory_chunk_t) + FENCE_SIZE);
    }
    struct memory_chunk_t* current_chunk = memory_manager.first_memory_chunk;

    //kolejne wywolania
    while(current_chunk!=memory_manager.tail){
        if(current_chunk->size>=size && current_chunk->free){
            current_chunk->free = 0;
            current_chunk->size = size;
            set_fence((uint8_t*)current_chunk+sizeof(struct memory_chunk_t));
            set_fence((uint8_t*)current_chunk+sizeof(struct memory_chunk_t)+ current_chunk->size+FENCE_SIZE);
            if(current_chunk->next!=memory_manager.tail) {
                size_t remaining_size = (size_t) ((uint8_t *) current_chunk->next - (uint8_t *) current_chunk -
                                                  block_size(current_chunk));
                if (remaining_size >= block_size(memory_manager.head)) {
                    size_t new_block_size = remaining_size - block_size(memory_manager.head);
                    struct memory_chunk_t *new_block = add_new_block(new_block_size, current_chunk->next);
                    new_block->free = 1;
                }
            }
            set_control_sum(current_chunk);
            return (void*)((uint8_t*)current_chunk+sizeof(struct memory_chunk_t) + FENCE_SIZE);
        }
        current_chunk = current_chunk->next;
    }
    //dodanie nowego bloku
    struct memory_chunk_t* new_chunk = add_new_block(size, memory_manager.tail);
    if(new_chunk==NULL){
        if(heap_expand(size) == -1){
            return NULL;
        }
        else{
            new_chunk = add_new_block(size, memory_manager.tail);
        }
    }
    set_control_sum(new_chunk);
    return (void*)((uint8_t*)new_chunk+sizeof(struct memory_chunk_t) + FENCE_SIZE);
}

void* heap_calloc(size_t number, size_t size){
    if(number<1 || size<1){
        return NULL;
    }
    void* data_block = heap_malloc(number*size);
    if(data_block==NULL){
        return NULL;
    }
    memset(data_block, 0, number*size);
    return data_block;
}

void* heap_realloc(void* memblock, size_t size) {
    if (memblock != NULL && get_pointer_type(memblock) != pointer_valid)
        return NULL;

    if (memblock == NULL)
        return heap_malloc(size);

    if (size < 1) {
        heap_free(memblock);
        return NULL;
    }

    struct memory_chunk_t* current = (struct memory_chunk_t*)((uint8_t*)memblock - FENCE_SIZE - sizeof(struct memory_chunk_t));

    if (current->size == size)
        return memblock;

    if (size < current->size) {
        current->size = size;
        set_fence((uint8_t*)current + sizeof(struct memory_chunk_t));
        set_fence((uint8_t*)current + sizeof(struct memory_chunk_t) + FENCE_SIZE + size);
        set_control_sum(current);
        split_block(current);
        return memblock;
    }

    if (current->next != memory_manager.tail && current->next->free && (current->size + block_size(current->next)>=size)) {
        coalesce_next(current);
        if (current->size >= size) {
            current->size = size;
            set_fence((uint8_t*)current + sizeof(struct memory_chunk_t));
            set_fence((uint8_t*)current + sizeof(struct memory_chunk_t) + FENCE_SIZE + size);
            set_control_sum(current);
            split_block(current);
            return memblock;
        }
    }

    if (current->next == memory_manager.tail) {
        size_t rest = size - current->size;
        if (heap_expand(rest) == 0) {
            current->size = size;
            set_fence((uint8_t*)current + sizeof(struct memory_chunk_t));
            set_fence((uint8_t*)current + sizeof(struct memory_chunk_t) + FENCE_SIZE + size);
            set_control_sum(current);
            return memblock;
        }
    }

    void* new_chunk = heap_malloc(size);
    if (new_chunk == NULL)
        return NULL;


    memcpy((uint8_t*)new_chunk, (uint8_t*)memblock, current->size);
    heap_free(memblock);
    return new_chunk;
}

void heap_free(void* memblock){
    if(memblock==NULL || get_pointer_type(memblock) != pointer_valid)return;
    struct memory_chunk_t* current = (struct memory_chunk_t*)((uint8_t*)memblock - sizeof(struct memory_chunk_t) - FENCE_SIZE);
    if(current->free){
        return ;
    }
    current->free = 1;
    set_control_sum(current);
    if(current->next!=memory_manager.tail && current->next->free){
        coalesce_next(current);
    }
    if(current->prev!=memory_manager.head && current->prev->free){
        coalesce_next(current->prev);
    }
    size_t remaining_size = (uint8_t*)current->next - ((uint8_t*)current+sizeof(struct memory_chunk_t) + 2* FENCE_SIZE + current->size);
    current->size += remaining_size;
    set_control_sum(current);
}

int heap_validate(void){
    if(memory_manager.memory_start == NULL || memory_manager.memory_size == 0 || memory_manager.head == NULL || memory_manager.tail == NULL){
        return 2;
    }

    uint8_t* heap_start = (uint8_t*)memory_manager.memory_start;
    uint8_t* heap_end = (uint8_t*)memory_manager.memory_start + memory_manager.memory_size;

    if(memory_manager.first_memory_chunk == NULL){
        if(memory_manager.head->next!=NULL || memory_manager.tail->prev!=NULL){
            return 3;
        }
        return 0;
    }

    struct memory_chunk_t* curr = memory_manager.first_memory_chunk;
    while(curr!=NULL && curr!=memory_manager.tail){
        if ((uint8_t*)curr < heap_start || (uint8_t*)curr + sizeof(struct memory_chunk_t) > heap_end){
            return 3;
        }
        if (curr->next != NULL && curr->next != memory_manager.tail && ((uint8_t*)curr->next < heap_start || (uint8_t*)curr->next > heap_end)){
            return 3;
        }
        if (curr->prev != NULL && curr->prev != memory_manager.head && ((uint8_t*)curr->prev < heap_start || (uint8_t*)curr->prev > heap_end)){
            return 3;
        }
        if (curr->next != NULL && curr->next->prev != curr){
            return 3;
        }
        if((curr->free != 0 && curr->free != 1)){
            return 3;
        }
        if(curr->size>(size_t)(heap_end-(uint8_t*)curr)){
            return 3;
        }
        if(curr->prev!=NULL && curr->prev->next != curr){
            return 3;
        }

        if(curr->control_sum!= calculate_control_sum(curr)){
            return 3;
        }

        if (!curr->free) {
            uint8_t* data_start = (uint8_t*)curr + sizeof(struct memory_chunk_t);
            uint8_t* data_end   = data_start + 2 * FENCE_SIZE + curr->size;
            if (data_end > heap_end) {
                return 1;
            }
            if (check_fence(data_start) != 0){
                return 1;
            }
            if (check_fence(data_start + FENCE_SIZE + curr->size) != 0){
                return 1;
            }
        }

        curr = curr->next;
    }
    return 0;
}

size_t heap_get_largest_used_block_size(void){
    if(heap_validate()!=0 || memory_manager.first_memory_chunk==NULL || memory_manager.head == NULL || memory_manager.tail == NULL){
        return 0;
    }
    size_t biggest_block = 0;
    struct memory_chunk_t* curr = memory_manager.first_memory_chunk;
    while(curr!=memory_manager.tail){
        if(curr->size>biggest_block && curr->free == 0){
            biggest_block = curr->size;
        }
        curr = curr->next;
    }
    return biggest_block;
}

enum pointer_type_t get_pointer_type(const void* const pointer){
    if(pointer==NULL){
        return pointer_null;
    }
    if(memory_manager.memory_start==NULL || memory_manager.first_memory_chunk==NULL){
        return pointer_unallocated;
    }
    if(heap_validate()!=0){
        return pointer_heap_corrupted;
    }
    struct memory_chunk_t* curr = memory_manager.first_memory_chunk;
    while(curr!=memory_manager.tail){
        if(curr->free && ((uint8_t*)pointer>=(uint8_t*)curr && (uint8_t*)pointer<(uint8_t*)curr + block_size(curr))) {
            return pointer_unallocated;
        }
        uint8_t* control_block = (uint8_t*)curr;
        uint8_t* head_fence = control_block + sizeof(struct memory_chunk_t);
        uint8_t* data_block = head_fence + FENCE_SIZE;
        uint8_t* tail_fence = data_block + curr->size;
        uint8_t* last_byte = tail_fence + FENCE_SIZE;

        if ((uint8_t*)pointer >= control_block && (uint8_t*)pointer < head_fence)
            return pointer_control_block;
        if ((uint8_t*)pointer >= head_fence && (uint8_t*)pointer < data_block)
            return pointer_inside_fences;
        if ((uint8_t*)pointer == data_block)
            return pointer_valid;
        if ((uint8_t*)pointer > data_block && (uint8_t*)pointer < tail_fence)
            return pointer_inside_data_block;
        if ((uint8_t*)pointer >= tail_fence && (uint8_t*)pointer < last_byte)
            return pointer_inside_fences;
        curr = curr->next;
    }
    return pointer_unallocated;
}
