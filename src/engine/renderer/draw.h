#ifndef DRAW_H
#define DRAW_H
#include "vulkan.h"
#include <engine/model.h>

typedef struct PDrawModelCommand{
  PModel* model;
  VkPipelineLayout layout;
  VkCommandBuffer command_buffer;
  uint32_t image_index;
} PDrawModelCommand;

void pe_vk_draw_simple_model(int i);
void pe_vk_draw_frame();
void pe_vk_draw_commands(VkCommandBuffer* cmd_buffer , uint32_t index);
void pe_vk_draw_model(PDrawModelCommand* draw_model);

void pe_vk_draw_model_instanced(PDrawModelCommand *draw_model,
                                VkBuffer instance_buffer,
                                uint32_t instance_count);

#endif
