#ifndef RENDER_VK_SWAPCHAIN
#define RENDER_VK_SWAPCHAIN

#include <vulkan/vulkan.h>
#include <engine/images.h>

typedef struct PRenderTarget PRenderTarget ;

void pe_vk_create_swapchain(PRenderTarget* target);

//INFO minImageCount is a floor, not the answer: the driver is free to hand
//back more images than were asked for, and vkAcquireNextImageKHR then returns
//indices up to its own count. these arrays have to hold whatever it made
#define PE_VK_MAX_SWAPCHAIN_IMAGES 8

#endif
