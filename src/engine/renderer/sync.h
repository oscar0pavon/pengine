#ifndef VKSYNC_H
#define VKSYNC_H

#include <vulkan/vulkan.h>

#include "swap_chain.h"

typedef struct PRenderTarget PRenderTarget;

//INFO how many frames the CPU may record ahead of the GPU. one of everything
//meant the CPU sat in vkWaitForFences until the GPU had finished the frame it
//had just submitted, so neither side ever had work queued behind it
#define PE_VK_FRAMES_IN_FLIGHT 1

void pe_vk_semaphores_create(PRenderTarget *target);

void pe_vk_end_sync(PRenderTarget *target);


#endif
