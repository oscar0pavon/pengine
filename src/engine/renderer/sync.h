#ifndef VKSYNC_H
#define VKSYNC_H

#include <vulkan/vulkan.h>


extern VkSemaphore pe_vk_semaphore_images_available;
extern VkSemaphore pe_vk_semaphore_render_finished;
extern VkFence pe_vk_fence_in_flight;

void pe_vk_semaphores_create(void);

void pe_vk_end_sync(void);


#endif
