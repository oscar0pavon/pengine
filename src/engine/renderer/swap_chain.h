#ifndef RENDER_VK_SWAPCHAIN
#define RENDER_VK_SWAPCHAIN

#include <vulkan/vulkan.h>
#include <engine/array.h>
#include <engine/images.h>

void pe_vk_create_swapchain();

extern VkSwapchainKHR pe_vk_swap_chain;

extern VkFormat pe_vk_swch_format;
extern VkExtent2D pe_vk_swch_extent;

extern u32 pe_vk_swapchain_image_count;

extern VkImage pe_vk_swch_images[4];
extern PTexture pe_vk_exportable_images[4];
extern int pe_vk_exportable_images_id[4];

#endif
