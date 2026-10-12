#ifndef PE_VK_QUEUES_H
#define PE_VK_QUEUES_H

#include <vulkan/vulkan.h>

extern VkDeviceQueueCreateInfo queues_creates_infos[2];

void pe_vk_queue_families_support();

#endif
