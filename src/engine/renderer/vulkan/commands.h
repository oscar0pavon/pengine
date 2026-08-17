#include <engine/renderer/vulkan/vulkan.h>
#include <engine/array.h>

VkCommandPool pe_vk_commands_pool;

Array pe_vk_command_buffers;
void pe_vk_record_commands_buffer(int i);
void pe_vk_command_init();
void pe_vk_commands_pool_init();
VkCommandBuffer pe_vk_begin_single_time_cmd();
void pe_vk_end_single_time_cmd(VkCommandBuffer buffer);
