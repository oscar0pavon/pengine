#ifndef COMMANDS_H
#define COMMANDS_H

#include <vulkan/vulkan.h>
#include <engine/array.h>
#include "vulkan.h"
#include "render_target.h"

extern VkCommandPool pe_vk_commands_pool;

void pe_vk_command_init(PRenderTarget *target);

void pe_vk_commands_pool_init();

//INFO separate from pe_vk_commands_pool: that one is shared by every one-shot
//upload (pe_vk_begin_single_time_cmd), this one belongs to a single target's
//per-frame command buffers
void pe_vk_target_command_pool_init(PRenderTarget *target);

VkCommandBuffer pe_vk_begin_single_time_cmd();

void pe_vk_end_single_time_cmd(VkCommandBuffer buffer);

void pe_vk_end_command(VkCommandBuffer command);

VkCommandBuffer pe_vk_start_record_command(PRenderTarget *target,
                                           int swapchain_image_index);

void pe_vk_clean_commands();

#endif

