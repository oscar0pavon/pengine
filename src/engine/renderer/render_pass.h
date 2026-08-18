#ifndef RENDER_PASS_H
#define RENDER_PASS_H

#include <vulkan/vulkan.h>

void pe_vk_create_render_pass();

void pe_vk_start_render_pass(VkCommandBuffer command, int i);

#endif
