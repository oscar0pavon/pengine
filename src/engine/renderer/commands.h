#ifndef COMMANDS_H
#define COMMANDS_H

#include <vulkan/vulkan.h>
#include <engine/array.h>
#include "vulkan.h"

extern VkCommandPool pe_vk_commands_pool;

extern Array pe_vk_command_buffers;

void pe_vk_command_init();

void pe_vk_commands_pool_init();

VkCommandBuffer pe_vk_begin_single_time_cmd();

void pe_vk_end_single_time_cmd(VkCommandBuffer buffer);

void pe_vk_end_command(VkCommandBuffer command);

VkCommandBuffer pe_vk_start_record_command(int swapchain_image_index);

void pe_vk_clean_commands();

#endif

