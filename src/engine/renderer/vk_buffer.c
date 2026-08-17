#include "vk_buffer.h"
#include "commands.h"
#include "engine/macros.h"
#include "engine/renderer/vulkan/vulkan.h"
#include "vk_memory.h"
#include <engine/log.h>
#include <engine/macros.h>
#include <vulkan/vulkan_core.h>

void pe_vk_buffer_create(PBufferCreateInfo *buffer_info) {
  VkBufferCreateInfo info;
  ZERO(info);
  info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  info.size = buffer_info->size;
  info.usage = buffer_info->usage;
  info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkBuffer buffer;
  ZERO(buffer);
  VKVALID(vkCreateBuffer(vk_device, &info, NULL, &buffer),
          "Can't create buffer");

  VkMemoryRequirements requirement = pe_vk_memory_get_requirements(buffer);
  VkDeviceMemory memory = pe_vk_memory_allocate(requirement);

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
