#ifndef RENDER_VK_SWAPCHAIN
#define RENDER_VK_SWAPCHAIN

#include <vulkan/vulkan.h>
#include <engine/array.h>
VkSwapchainKHR pe_vk_swap_chain;

VkFormat pe_vk_swch_format;
VkExtent2D pe_vk_swch_extent;

void pe_vk_swch_create();

VkImage pe_vk_swch_images[4];

#endif
