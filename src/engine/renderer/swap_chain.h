#ifndef RENDER_VK_SWAPCHAIN
#define RENDER_VK_SWAPCHAIN

#include <vulkan/vulkan.h>
#include <engine/images.h>

typedef struct PRenderTarget PRenderTarget ;

void pe_vk_create_swapchain(PRenderTarget* target);

//INFO rebuilds a target and everything sized from its extent: the swap chain,
//its image views, the colour and depth attachments, the framebuffers, the per
//image command buffers and semaphores, and the target's own camera. A window
//that has been resized picks its new size up from pe_window_width/height, so
//those go first - a wayland surface leaves the extent to the client, and the
//swap chain is what everything else measures itself against
//
//It waits for the device to go idle, so it belongs on the thread that draws,
//between two frames - never in a window event callback
void pe_vk_recreate_swapchain(PRenderTarget* target);

//INFO minImageCount is a floor, not the answer: the driver is free to hand
//back more images than were asked for, and vkAcquireNextImageKHR then returns
//indices up to its own count. these arrays have to hold whatever it made
#define PE_VK_MAX_SWAPCHAIN_IMAGES 8

#endif
