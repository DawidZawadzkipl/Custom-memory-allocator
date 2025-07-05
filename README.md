# Custom Memory Allocator
### A custom implementation of dynamic memory allocation functions (malloc, free, realloc, calloc) in C, built from scratch using low-level system calls.
## 🚀 Features

- Complete malloc family: heap_malloc(), heap_free(), heap_realloc(), heap_calloc()
- Memory safety: Buffer overflow protection with guard fences
- Corruption detection: Control sum validation and heap integrity checks
- Memory coalescing: Automatic merging of adjacent free blocks
- Block splitting: Efficient splitting of large blocks for smaller allocations

## 🛠️ How It Works
The allocator manages memory using a doubly-linked list of memory chunks, where each chunk contains:

- Control block: Metadata (size, free status, pointers to adjacent chunks)
- Guard fences: Protection against buffer overflows (before and after data)
- Data area: The actual user data
- Control sum: Integrity verification
```
[CONTROL][FENCE][USER DATA][FENCE][CONTROL][FENCE][USER DATA][FENCE]...
```
### Key Algorithms

- First-fit allocation: Searches for the first available block that fits
- Coalescing: Merges adjacent free blocks to reduce fragmentation
- Block splitting: Divides large blocks when smaller allocation requested
- Heap expansion: Automatically grows heap using sbrk() when needed

## 📋 API Reference
```
// Memory allocation
void* heap_malloc(size_t size);
void* heap_calloc(size_t number, size_t size);
void* heap_realloc(void* memblock, size_t size);
void  heap_free(void* memblock);

// Heap management
int   heap_setup(void);
void  heap_clean(void);
int   heap_validate(void);

// Debugging utilities
enum pointer_type_t get_pointer_type(const void* const pointer);
size_t heap_get_largest_used_block_size(void);
```
## 🔍 Pointer Types
The allocator can identify different types of pointers:

- `pointer_null` - NULL pointer
- `pointer_valid`- Valid pointer to allocated memory
- `pointer_unallocated` - Pointer to freed or never-allocated memory
- `pointer_inside_data_block` - Pointer inside allocated block (not to start)
- `pointer_inside_fences` - Pointer to guard fence area
- `pointer_control_block` - Pointer to internal control structure
- `pointer_heap_corrupted` - Heap integrity violation detected

## 🚦 Usage Example
```
#include "heap.h"

int main() {
    // Initialize heap
    if (heap_setup() != 0) {
        printf("Failed to initialize heap\n");
        return -1;
    }
    
    // Allocate memory
    char* buffer = (char*)heap_malloc(100);
    if (buffer == NULL) {
        printf("Allocation failed\n");
        return -1;
    }
    
    // Use memory safely
    strcpy(buffer, "Hello, World!");
    printf("%s\n", buffer);
    
    // Reallocate to larger size
    buffer = (char*)heap_realloc(buffer, 200);
    
    // Free memory
    heap_free(buffer);
    
    // Validate heap integrity
    if (heap_validate() != 0) {
        printf("Heap corruption detected!\n");
    }
    
    // Cleanup
    heap_clean();
    return 0;
}
```

## ⚠️ Limitations

- Platform dependent: Requires sbrk() system call (Unix/Linux only)
- Single-threaded: Not thread-safe (no synchronization)
- No malloc compatibility: Cannot be used alongside system malloc()
- Educational purpose: Not optimized for production use

## 🔬 Technical Details

- Alignment: All allocations are aligned to 4-byte boundaries
- Minimum allocation: Overhead of ~16 bytes per allocation
- Fragmentation: Uses first-fit with coalescing to minimize fragmentation
- Growth strategy: Heap grows in 4KB pages as needed

## 📚 Educational Value
This project demonstrates:

- Low-level memory management concepts
- Linked list data structures in systems programming
- Buffer overflow protection techniques
- Memory corruption detection methods
- System call usage (sbrk())

## 📄 License
This project is open source and available under the MIT License.
### 👨‍💻 Author
Created as part of a operating systems course to understand memory allocation internals.

Note: This allocator is designed for educational purposes. For production applications, use the standard library malloc() family or specialized allocators.
