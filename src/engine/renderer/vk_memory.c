#include "vulkan.h"
#include <engine/macros.h>
#include <stdio.h>

VkMemoryRequirements pe_vk_memory_get_requirements(VkBuffer buffer){
    VkMemoryRequirements requirements;
    vkGetBufferMemoryRequirements(vk_device,buffer,&requirements);
    return requirements;
}

uint32_t pe_vk_memory_find_type(uint32_t type_filter, VkMemoryPropertyFlags flags){
    VkPhysicalDeviceMemoryProperties properties;
    vkGetPhysicalDeviceMemoryProperties(vk_physical_device,&properties);

    for (uint32_t i = 0; i < properties.memoryTypeCount; i++) {
        if ((type_filter & (1 << i)) && (properties.memoryTypes[i].propertyFlags & flags) == flags) {
            return i;
        }
    }
}

VkDeviceMemory pe_vk_allocate_memory(VkMemoryRequirements requirements) {

  uint32_t index_type = pe_vk_memory_find_type(
      requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);


  VkMemoryAllocateInfo info = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                               .allocationSize = requirements.size,
                               .memoryTypeIndex = index_type};

  VkDeviceMemory memory;
  vkAllocateMemory(vk_device, &info, NULL, &memory);
  //printf("Allocated memory %p\n", memory);
  return memory;
}
