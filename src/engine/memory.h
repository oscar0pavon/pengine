#ifndef ENGINE_MEMORY
#define ENGINE_MEMORY

#include <string.h>

#define INIT_MEMORY 750000000


typedef struct StackMemory{
    int used;
    int marker;
    int available;
    int previous_marker;
    void* memory;
}StackMemory;

typedef struct PoolMemory{
    int used;
    int marker;
    int available;
    int previous_marker;
    void* memory;
}PoolMemory;

void pe_init_memory();
void* allocate_memory(int size);
void clear_engine_memory();
void engine_memory_free_to_marker(int);

int engine_memory_mark();

void* allocate_stack_memory(StackMemory* stack, int bytes_size);
void free_stack_to_market(StackMemory* stack);

void* allocate_stack_memory_alignmed(int bytes_size, int alignment);

extern int memory_used;
extern int memory_marker;
extern int previous_marker;
extern int actual_free_memory;

extern void* engine_memory;


extern StackMemory vertex_memory;
extern StackMemory engine_stack_memory;

extern PoolMemory arrays_memory;

#endif
