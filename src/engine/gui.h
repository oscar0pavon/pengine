#ifndef ENGINE_GUI_H
#define ENGINE_GUI_H

#include <cglm/cglm.h>
#include <vulkan/vulkan_core.h>

#include "model.h"

typedef struct Button {
  PModel model;
  vec4 color;
  vec2 position;
  vec2 size;
  bool pressed;
  void (*action)(void);
} Button;

void pe_gui_init();

Button *pe_gui_button_new(vec2 position, vec2 size, vec4 color,
                          void (*action)(void));

void pe_gui_update();

void pe_gui_draw(VkCommandBuffer *cmd_buffer, uint32_t image_index);

#endif
