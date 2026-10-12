#ifndef PE_VK_MEMORY_H
#define PE_VK_MEMORY_H

#include <string.h>
#include "vulkan.h"
VkDeviceMemory pe_vk_allocate_memory(VkMemoryRequirements requirements);
VkMemoryRequirements pe_vk_memory_get_requirements(VkBuffer buffer);
uint32_t pe_vk_memory_find_type(uint32_t type_filter, VkMemoryPropertyFlags flags);

#endif // !PE_VK_MEMORY_H
