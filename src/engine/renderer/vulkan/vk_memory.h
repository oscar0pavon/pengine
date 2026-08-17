
#include <string.h>
#include "vulkan.h"
VkDeviceMemory pe_vk_memory_allocate(VkMemoryRequirements requirements);
VkMemoryRequirements pe_vk_memory_get_requirements(VkBuffer buffer);
uint32_t pe_vk_memory_find_type(uint32_t type_filter, VkMemoryPropertyFlags flags);
