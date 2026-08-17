#include <engine/renderer/vulkan/vulkan.h>
#include <engine/model.h>
void pe_vk_draw_simple_model(int i);
void pe_vk_draw_model(int i , PModel* model);
void pe_vk_draw_frame();
void pe_vk_draw_commands(VkCommandBuffer* cmd_buffer , uint32_t index);
