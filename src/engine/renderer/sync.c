
#include "sync.h"
#include "swap_chain.h"
#include "vulkan.h"
#include <engine/macros.h>
#include <vulkan/vulkan_core.h>

VkSemaphore pe_vk_semaphore_images_available[PE_VK_FRAMES_IN_FLIGHT];
VkSemaphore pe_vk_semaphore_render_finished[PE_VK_MAX_SWAPCHAIN_IMAGES];
VkFence pe_vk_fence_in_flight[PE_VK_FRAMES_IN_FLIGHT];
VkFence pe_vk_fence_image_in_flight[PE_VK_MAX_SWAPCHAIN_IMAGES];

uint32_t pe_vk_current_frame;

void pe_vk_semaphores_create(PRenderTarget *target){

  VkSemaphoreCreateInfo info;
  ZERO(info);
  info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  //INFO created signalled, or the first vkWaitForFences on each of them would
  //wait for a frame that was never submitted
  VkFenceCreateInfo fence_info = {
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    .flags = VK_FENCE_CREATE_SIGNALED_BIT
  };

  for (uint32_t i = 0; i < PE_VK_FRAMES_IN_FLIGHT; i++) {
    vkCreateSemaphore(vk_device, &info, NULL,
                      &target->semaphore_images_available[i]);
    vkCreateFence(vk_device, &fence_info, NULL, &target->fence_in_flight[i]);
  }

  //one per image the swap chain actually made, which is not the count that was
  //requested - pe_vk_create_swapchain() writes the real one
  for (uint32_t i = 0; i < target->images_count; i++)
    vkCreateSemaphore(vk_device, &info, NULL,
                      &target->semaphore_render_finished[i]);

  ZERO(target->fence_image_in_flight);

  target->current_frame = 0;
}

void pe_vk_end_sync(PRenderTarget *target){

  for (uint32_t i = 0; i < PE_VK_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(vk_device, target->semaphore_images_available[i], NULL);
    vkDestroyFence(vk_device, target->fence_in_flight[i], NULL);
  }

  for (uint32_t i = 0; i < target->images_count; i++)
    vkDestroySemaphore(vk_device, target->semaphore_render_finished[i], NULL);
}
