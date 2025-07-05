#include "heap.h"

int main() {
    printf("Custom Memory Allocator Demo\n");

    if (heap_setup() != 0) {
        printf(" Failed to initialize heap\n");
        return -1;
    }
    printf("Heap initialized\n");

    char* buffer = (char*)heap_malloc(50);
    if (buffer) {
        strcpy(buffer, "Custom malloc test!");
        printf("Allocated and wrote: %s\n", buffer);
        heap_free(buffer);
        printf("Memory freed\n");
    }

    printf("Largest used block: %zu bytes\n", heap_get_largest_used_block_size());

    heap_clean();
    printf("Heap cleaned up\n");
    return 0;
}