#include "vk_buffer.h"
#include "commands.h"
#include "engine/macros.h"
#include "vulkan.h"
#include "vk_memory.h"
#include <engine/log.h>
#include <engine/macros.h>
#include <vulkan/vulkan_core.h>

void pe_vk_create_buffer_memory(PBufferCreateInfo *buffer_info) {
  VkBufferCreateInfo info = {

      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = buffer_info->size,
      .usage = buffer_info->usage,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE

  };

  VkBuffer buffer;
  VKVALID(vkCreateBuffer(vk_device, &info, NULL, &buffer),
          "Can't create buffer");

  array_add(&buffers,&buffer);

  VkMemoryRequirements requirement = pe_vk_memory_get_requirements(buffer);
  VkDeviceMemory memory = pe_vk_allocate_memory(requirement);

  VKVALID(vkBindBufferMemory(vk_device, buffer, memory, 0),
          "Can't bind memory");

  buffer_info->buffer_memory = memory;
  buffer_info->buffer = buffer;
}

void pe_vk_copy_buffer(VkBuffer source, VkBuffer destination,
                       VkDeviceSize size) {
  VkCommandBuffer command = pe_vk_begin_single_time_cmd();

  VkBufferCopy copy_region;
  ZERO(copy_region);
  copy_region.size = size;

  vkCmdCopyBuffer(command, source, destination, 1, &copy_region);

  pe_vk_end_single_time_cmd(command);
}

PBuffer pe_vk_create_buffer(u64 size, void* data , VkBufferUsageFlagBits type) {

  PBufferCreateInfo info = {

      .usage = type,
      .properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      .size = size

  };

  pe_vk_create_buffer_memory(&info);

  void *vulkan_memory;
  vkMapMemory(vk_device, info.buffer_memory, 0, size, 0, &vulkan_memory);
  memcpy(vulkan_memory, data, size);
  vkUnmapMemory(vk_device, info.buffer_memory);


  PBuffer buffer = {
    .buffer = info.buffer,
    .memory = info.buffer_memory
  };

  return buffer;
}

void pe_vk_update_buffer(PBuffer *buffer, void *data, u64 size) {

  void *vulkan_memory;
  vkMapMemory(vk_device, buffer->memory, 0, size, 0, &vulkan_memory);
  memcpy(vulkan_memory, data, size);
  vkUnmapMemory(vk_device, buffer->memory);
}
