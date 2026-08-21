#ifndef VKSYNC_H
#define VKSYNC_H

#include <vulkan/vulkan.h>

#include "swap_chain.h"

typedef struct PRenderTarget PRenderTarget;

//INFO how many frames the CPU may record ahead of the GPU. one of everything
//meant the CPU sat in vkWaitForFences until the GPU had finished the frame it
//had just submitted, so neither side ever had work queued behind it
#define PE_VK_FRAMES_IN_FLIGHT 1

//waited on by the submit that renders into the image the acquire returned, so
//there is one per frame in flight - the frame owns it from acquire to submit
extern VkSemaphore pe_vk_semaphore_images_available[PE_VK_FRAMES_IN_FLIGHT];

//INFO this one is per swap chain image and not per frame: it is still pending
//inside vkQueuePresentKHR when the next frame starts, and a frame reusing its
//own would wait on a semaphore the presentation engine has not signalled yet
extern VkSemaphore pe_vk_semaphore_render_finished[PE_VK_MAX_SWAPCHAIN_IMAGES];

extern VkFence pe_vk_fence_in_flight[PE_VK_FRAMES_IN_FLIGHT];

//INFO not owned here: each entry is whichever frame's fence last rendered into
//that image, or VK_NULL_HANDLE if none has. the image's command buffer and the
//uniform buffers indexed by image_index are only safe to rewrite once it signals
extern VkFence pe_vk_fence_image_in_flight[PE_VK_MAX_SWAPCHAIN_IMAGES];

extern uint32_t pe_vk_current_frame;

void pe_vk_semaphores_create(PRenderTarget *target);

void pe_vk_end_sync(PRenderTarget *target);


#endif
