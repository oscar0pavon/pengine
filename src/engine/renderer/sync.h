#include <engine/renderer/vulkan/vulkan.h>


VkSemaphore pe_vk_semaphore_images_available;
VkSemaphore pe_vk_semaphore_render_finished;
VkFence pe_vk_fence_in_flight;

void pe_vk_semaphores_create();
