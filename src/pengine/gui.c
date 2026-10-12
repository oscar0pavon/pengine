#include "gui.h"

#include <string.h>

#include "array.h"
#include "engine2d.h"
#include "files.h"
#include "input.h"
#include "utils.h"

#include "renderer/descriptor_set.h"
#include "renderer/draw.h"
#include "renderer/shaders.h"
#include "renderer/uniform_buffer.h"

static Array buttons;
static PModel button_quad;

//INFO pe_2d_create_quad_geometry() builds its descriptor sets for the textured
//layout, which the flat colour pipeline can't bind. the quad is only the source
//of the shared vertex and index buffers; every button gets its own compatible
//sets through pe_vk_model_instance()
void pe_gui_init() {
  array_init(&buttons, sizeof(Button), 20);

  PCreateShaderInfo shader_info;
  ZERO(shader_info);
  shader_info.out_shader = &button_quad.shader;
  shader_info.vertex_path = file_color_vert_spv;
  shader_info.fragment_path = file_color_frag_spv;
  shader_info.layout = pe_vk_pipeline_layout_with_descriptors;
  pe_vk_create_shader(&shader_info);

  pe_2d_create_quad_geometry(&button_quad);
}

Button *pe_gui_button_new(vec2 position, vec2 size, vec4 color,
                          void (*action)(void)) {
  Button button;
  ZERO(button);
  pe_vk_model_instance(&button.model, &button_quad);

  glm_vec2_copy(position, button.position);
  glm_vec2_copy(size, button.size);
  glm_vec4_copy(color, button.color);
  button.action = action;

  array_add(&buttons, &button);
  return array_get_last(&buttons);
}

static bool button_contains_pointer(Button *button) {
  return touch_position_x >= button->position[0] &&
         touch_position_x <= button->position[0] + button->size[0] &&
         touch_position_y >= button->position[1] &&
         touch_position_y <= button->position[1] + button->size[1];
}

//the action runs once per press, not once per frame the pointer stays down
void pe_gui_update() {
  for (size_t i = 0; i < buttons.count; i++) {
    Button *button = array_get(&buttons, i);

    bool inside = button_contains_pointer(button);
    if (inside && !button->pressed && button->action)
      button->action();

    button->pressed = inside;
  }
}

void pe_gui_draw(VkCommandBuffer *cmd_buffer, uint32_t image_index) {
  for (size_t i = 0; i < buttons.count; i++) {
    Button *button = array_get(&buttons, i);
    PUniformBufferObject *ubo = &button->model.uniform_buffer_object;

    glm_mat4_identity(ubo->view);
    glm_vec4_copy(button->color, ubo->color);
    pe_2d_draw(&button->model, image_index, button->position, button->size);

    PDrawModelCommand draw;
    ZERO(draw);
    draw.model = &button->model;
    draw.layout = pe_vk_pipeline_layout_with_descriptors;
    draw.command_buffer = *cmd_buffer;
    draw.image_index = image_index;
    pe_vk_draw_model(&draw);
  }
}
