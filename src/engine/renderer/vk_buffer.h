#ifndef VK_BUFFER
#define VK_BUFFER
#include "vulkan.h"

typedef struct PBufferCreateInfo {
  VkDeviceSize size;
  VkBufferUsageFlags usage;
  VkMemoryPropertyFlags properties;
  VkBuffer buffer;
  VkDeviceMemory buffer_memory;
} PBufferCreateInfo;

typedef struct PBuffer {
  VkBuffer buffer;
  VkDeviceMemory memory;
} PBuffer;

void pe_vk_create_buffer_memory(PBufferCreateInfo *buffer_info);

void pe_vk_copy_buffer(VkBuffer source, VkBuffer destination,
                       VkDeviceSize size);

PBuffer pe_vk_create_buffer(u64 size, void *data, VkBufferUsageFlagBits type);

//refill a buffer that is already allocated. only valid on the host visible
//memory pe_vk_create_buffer hands out, and only while the gpu is not reading
//it
void pe_vk_update_buffer(PBuffer *buffer, void *data, u64 size);

#endif
