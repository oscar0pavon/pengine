#ifndef PE_RENDER_TARGET
#define PE_RENDER_TARGET

#include "engine/camera.h"
#include "surface.h"
#include "swap_chain.h"
#include "sync.h"
#include <engine/types.h>

typedef struct PRenderTarget{

  VkSurfaceKHR surface;
  VkSwapchainKHR swap_chain;
  VkExtent2D extent;
  VkImage images[PE_VK_MAX_SWAPCHAIN_IMAGES];
  Array images_views;
  Array framebuffers;

  VkImage color_image;
  VkDeviceMemory color_memory;
  VkImageView color_image_view;

  VkCommandPool commands_pool;

  Array command_buffers;

  VkSemaphore semaphore_images_available[PE_VK_FRAMES_IN_FLIGHT];
  VkSemaphore semaphore_render_finished[PE_VK_MAX_SWAPCHAIN_IMAGES];
  VkFence fence_in_flight[PE_VK_FRAMES_IN_FLIGHT];
  VkFence fence_image_in_flight[PE_VK_MAX_SWAPCHAIN_IMAGES];

  VkViewport viewport;
  VkRect2D scissor;

  PCamera camera;

  u32 heigth;
  u32 width;
}PRenderTarget;

#endif
