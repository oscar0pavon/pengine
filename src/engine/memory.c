#include "memory.h"
#include <stdlib.h>
#include "log.h"

#include <stdbool.h>

bool memory_lock;

// VERY IMPORTANT
void pe_init_memory(){
    engine_memory = malloc(INIT_MEMORY);
    memset(engine_memory,0,INIT_MEMORY);
    actual_free_memory = INIT_MEMORY;
    memory_marker = 0;

}


void* allocate_stack_memory(StackMemory* stack, int bytes_size){
    if(stack->available > bytes_size){

        int expanded_size_bytes = bytes_size + 16;
        unsigned long int raw_memory = (unsigned long int)(stack->memory + stack->marker);

        int mask = (16 - 1);
        unsigned long int misalignment = (raw_memory & mask);
        unsigned long int adjustment = 16 - misalignment;

        unsigned long int aligned_adress = raw_memory + adjustment;

        stack->used += expanded_size_bytes;
        stack->available -= expanded_size_bytes;
        //void* allocated_memory = &stack->memory[stack->marker];
        stack->previous_marker = stack->marker;
        stack->marker += expanded_size_bytes;
        return (void*)aligned_adress;
    }   
    LOG("ERROR allocating stack memory\n");
    return NULL;
}



void free_stack_to_market(StackMemory* stack){
    stack->used += stack->marker;
    stack->available += stack->marker;
    stack->used -= stack->marker;
}

void* allocate_memory(int size){
    if(memory_lock==true){
        LOG("Memory locket\n");
    }
    while(memory_lock){}

    memory_lock = true;
    if(actual_free_memory > size){
        memory_used += size;
        actual_free_memory -= size;
        void* allocated_memory = (engine_memory + memory_marker);
        previous_marker = memory_marker;
        memory_marker += size;
        memory_lock = false;
        return allocated_memory;
    }   
    LOG("ERROR engine memory\n");
   
    return NULL;
}

void* allocate_stack_memory_alignmed(int bytes_size, int alignment){
    int expanded_size_bytes = bytes_size + alignment;
    unsigned long int raw_memory = (unsigned long int)allocate_memory(expanded_size_bytes);

    int mask = (alignment - 1);
    unsigned long int misalignment = (raw_memory & mask);
    unsigned long int adjustment = alignment - misalignment;

    unsigned long int aligned_adress = raw_memory + adjustment;
    return (void*)aligned_adress;
}

void clear_engine_memory(){
    free(engine_memory);
}

void engine_memory_free_to_marker(int marker){
    memory_used += memory_marker - marker;
    actual_free_memory += memory_marker - marker;
    memory_marker -= memory_marker - marker;
}   

int engine_memory_mark(){
    return memory_marker;
}
