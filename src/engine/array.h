#ifndef ARRAY_H
#define ARRAY_H

#include "../engine/numbers.h"
#include <stdbool.h>

typedef struct Array{
    bool initialized;
    bool isPointerToPointer;
    bool block;
    int element_capacity;
    u32 bytes_size;
    u32 count;
    u32 element_bytes_size;
    u32 bytes_capacity;
    void * data;
}Array;

int array_init(Array * array, u32 element_bytes_size, int count);

int array_new_pointer(Array* array,int count);

void array_add(Array* array, const void* element);

void array_add_pointer(Array* array, const void* element);

void *array_get_pointer(Array*,int index);

void *array_get(Array*,int index);

void array_clean(Array* array);

void array_remove_element(Array *array, void *pointer);

void array_resize(Array* array, int count);

void *array_get_last(Array* array);

//get the last element added
inline static void* array_pop(Array* array){
	return array_get(array,array->count-1);
}
#endif // !ARRAY_H
