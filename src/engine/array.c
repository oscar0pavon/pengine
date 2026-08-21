#include "array.h"
#include "interruptions.h"
#include "log.h"
#include "memory.h"
#include <engine/macros.h>
#include <stdio.h>
#include <string.h>

int array_init(Array *array, u32 element_bytes_size, int count) {
  ZERO(*(array));
  if (array == NULL) {
    LOG("Array is NULL");
    return -1;
  }

  if (array->initialized != true) {

  } else {
    // LOG("Array already initialized\n");
    return -1;
  }
  if (count < 0)
    count = 0;

  array->count = 0;
  array->bytes_size = 0;
  array->element_bytes_size = element_bytes_size;

  //INFO count is a starting capacity now, not a limit - array_add() grows past
  //it. it still decides how much gets copied on the first growth, so a sensible
  //guess is worth making
  array->data = allocate_stack_memory_alignmed(element_bytes_size * count, 16);
  if (array->data == NULL && count > 0) {
    LOG("Array init failed: no room for %i elements of %u bytes\n", count,
        element_bytes_size);
    return -1;
  }

  array->bytes_capacity = element_bytes_size * count;
  array->initialized = true;
  array->element_capacity = count;
  return 0;
}

int array_new_pointer(Array *array, int count) {
  array_init(array, sizeof(void *), count);
}

//INFO the arena is a bump allocator with no free, so growing means taking a
//bigger block and copying into it - the old one stays where it is. capacity
//doubles, so everything an array leaves behind over its whole life adds up to
//less than the size it ends at.
//
//IMPORTANT: growing moves the elements. a pointer from array_get() or
//array_get_last() taken before a growth points into the block that was left
//behind, so do not hold one across an array_add() on the same array.
static bool array_grow(Array *array, u32 needed_bytes) {

  u32 new_bytes_capacity = array->bytes_capacity;

  if (new_bytes_capacity < array->element_bytes_size)
    new_bytes_capacity = array->element_bytes_size;

  while (new_bytes_capacity < needed_bytes) {
    u32 doubled = new_bytes_capacity * 2;

    //stop doubling rather than wrapping round to a small capacity
    if (doubled < new_bytes_capacity) {
      new_bytes_capacity = needed_bytes;
      break;
    }
    new_bytes_capacity = doubled;
  }

  //allocate_memory() takes an int and tests actual_free_memory > size, so a
  //size past INT_MAX arrives negative, passes that test and hands back a block
  //that was never reserved
  if (new_bytes_capacity > INIT_MEMORY) {
    LOG("Array growth refused: %u bytes is larger than the engine arena\n",
        new_bytes_capacity);
    return false;
  }

  void *new_data = allocate_stack_memory_alignmed(new_bytes_capacity, 16);
  if (new_data == NULL) {
    LOG("Array growth failed: no room for %u bytes in the engine arena\n",
        new_bytes_capacity);
    return false;
  }

  memcpy(new_data, array->data, array->bytes_size);

  array->data = new_data;
  array->bytes_capacity = new_bytes_capacity;
  array->element_capacity = new_bytes_capacity / array->element_bytes_size;

  return true;
}

void array_resize(Array *array, int count) {
  if (count < 0)
    return;

  u32 needed_bytes = (u32)count * array->element_bytes_size;

  if (array->bytes_capacity < needed_bytes) {
    if (array_grow(array, needed_bytes) == false)
      return;
  }

  array->bytes_size = needed_bytes;
  array->count = count;
}

void array_copy(Array *source, Array *destination) {}

// maybe only work with pointer
void array_remove_element(Array *array, void *pointer) {
  char original_data[array->bytes_size];
  Array original_array;
  memcpy(&original_array, array, sizeof(Array));
  //sizeof(array->bytes_size) is 4 - the size of the u32 field, not the size it
  //holds. every element past the first four bytes was copied out of
  //uninitialised stack, so removing one element filled the array with garbage
  //pointers. tasks_for_draw is an array of Task*, so a client disconnecting
  //left the render loop dereferencing stack noise
  memcpy(&original_data, array->data, array->bytes_size);

  original_array.data = &original_data;

  array_clean(array);
  for (int i = 0; i < original_array.count; i++) {
    void *temp_pointer = array_get_pointer(&original_array, i);
    if (temp_pointer != pointer) {
      array_add_pointer(array, temp_pointer);
    }
  }

}

void array_add(Array *array, const void *element) {
  if (array->initialized == false) {
    LOG("Array not initialized\n");
    debug_break();
    ;
    return;
  }
  u32 needed_bytes = array->bytes_size + array->element_bytes_size;

  if (array->bytes_capacity < needed_bytes) {
    if (array_grow(array, needed_bytes) == false)
      return;
  }

  //the count == 0 case used to be written out separately, but its offset was
  //bytes_size, which is 0 then anyway
  memcpy(array->data + array->bytes_size, element, array->element_bytes_size);

  array->bytes_size = needed_bytes;
  array->count++;
}

void array_add_pointer(Array *array, const void *element) {
  for(int i = 0; i < array->count; i++){
    void *old_element = array_get_pointer(array, i);
    if(old_element == element)
      return;
  }
  array_add(array, &element);
}

void *array_get_last(Array *array) {
  if (array->count > 0) {
    return array_get(array, array->count - 1);
  } else
    return NULL;
}

void *array_get(Array *array, int index) {
  if (array->count == 0) {
    // LOG("Array is empty\n");
    return NULL;
  }
  if (array->initialized == false) {
    LOG("Array not initialized\n");
    debug_break();
    ;
    return NULL;
  }
  if (index > array->count - 1) {
    LOG("Element out of range, array count: %i , requested %i\n", array->count,
        index);
    //				debug_break();
    return NULL;
  }
  size_t offset = array->element_bytes_size;

  /*  if(array->isPointerToPointer){
       void* data = NULL;
       if(index == 0)
           data = &array->data[0];
       else
           data = &array->data[0] + (index*offset);
       return data;
   } */

  if (index == 0)
    return array->data;
  return array->data + (index * offset);
}

void *array_get_pointer(Array *array, int index) {
  void **ppointer = array_get(array, index);
  void *pointer = ppointer[0];
  return pointer;
}

void array_clean(Array *array) {
  array->count = 0;
  array->bytes_size = 0;
}
