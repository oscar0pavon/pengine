//
// Created by pavon on 6/23/19.
//

#include "gui.h"
#include "../ThirdParty/cglm/cglm.h"
#include "camera.h"

#include "input.h"

#include "shader.h"

#include "images.h"
#include <string.h>

#include "engine.h"

#include <engine/renderer/renderer.h>

GLuint logo_shader;

GLuint pe_gui_shader_vert;
GLuint pe_gui_shader_farg;

Array gui_vertex_array;

GLuint logo_texture_id;

Array buttons;

void create_gui_shaders() {
  for (size_t i = 0; i < buttons.count; i++) {

    size_t offset = buttons.element_bytes_size;

    Button *button = buttons.data + (i * offset);

    button->shader =
        create_engine_shader(pe_gui_shader_vert, pe_gui_shader_farg);
  }
}

void pe_gui_compile_shaders() {
  pe_gui_shader_vert = standart_vertex_shader;
  pe_gui_shader_farg = standart_fragment_shader;

  create_gui_shaders();
}

void check_if_pressed(struct Button *button) {
  u8 minx = button->position[0] - button->size[0];
  u8 maxx = button->position[0] + button->size[0];

  u8 miny = button->position[1] - button->size[1];
  u8 maxy = button->position[1] + button->size[1];

  if (minx <= touch_position_x && maxy >= touch_position_y) {
    if (miny <= touch_position_y && maxx >= touch_position_x) {
      button->pressed = true;
      LOG("button pressed\n");
    }
  } else {
    button->pressed = false;
  }
}

void two_dimension_screen_space_send_matrix(GLuint shader_id, vec2 size,
                                            vec2 position) {
  mat4 scale;
  glm_mat4_identity(scale);

  mat4 position_mat;
  glm_mat4_identity(position_mat);

  mat4 projection;
  glm_mat4_identity(projection);

  glm_ortho(0, camera_width_screen, camera_heigth_screen, 0, 0, 1, projection);

  glm_scale(scale, (vec3){size[0], size[1], 0});

  glm_translate(position_mat, (vec3){position[0], position[1], 0});

  mat4 model;
  glm_mul(position_mat, scale, model);

  mat4 model_projection;
  glm_mat4_identity(model_projection);

  glm_mul(projection, model, model_projection);

  GLint mvp_uniform = get_uniform_location(shader_id, "MVP");
  glUniformMatrix4fv(mvp_uniform, 1, GL_FALSE, &model_projection[0][0]);
  check_error("Screen space send mat");
}

void pe_buttons_draw() {

  for (size_t i = 0; i < buttons.count; i++) {

    Button *button = array_get(&buttons, i);

    DrawData data;
    data.shader = button->shader;
    data.vertex = UI_plane_vertex_buffer_id;
    data.texture = button->texture.id;

    pe_render_2d(&data, button->position, button->size, NULL);
  }
}

void create_vertex_buffer() {
  glGenBuffers(1, &UI_plane_vertex_buffer_id);
  glBindBuffer(GL_ARRAY_BUFFER, UI_plane_vertex_buffer_id);
  glBufferData(GL_ARRAY_BUFFER, gui_vertex_array.count * sizeof(struct Vertex),
               gui_vertex_array.data, GL_STATIC_DRAW);
  engine_memory_free_to_marker(previous_marker);
  // free(gui_vertex_array.vertices);

  // gui_vertex_array.vertices = NULL;
}

void init_button(Button *button, float position_x, float position_y,
                 float size_x, float size_y) {
  char name[] = "NewButton";
  memset(button->name, 0, strlen(button->name));
  memcpy(button->name, name, strlen(name));

  button->position[0] = position_x;

  float reasigned_size_x = camera_width_screen / 1280;
  float reasigned_size_y = camera_heigth_screen / 720;

  float aspect_ratio = camera_width_screen / camera_heigth_screen;
  float new_size_x = size_x * (reasigned_size_x + aspect_ratio);
  float new_size_y = size_y * (reasigned_size_y + aspect_ratio);

  button->size[0] = new_size_x;
  button->size[1] = new_size_y;

  button->original_position[0] = position_x;
  if (button->relative_to == POSITION_RELATIVE_LEFT_BOTTON) {
    button->original_position[1] = camera_heigth_screen - position_y;
    button->position[1] = camera_heigth_screen - position_y;
  } else {
    button->original_position[1] = position_y;
    button->position[1] = position_y;
  }
}

void init_gui_element_geometry() {
  struct Vertex vert1;
  init_vec3(-1.0F, 1.0, 0.0, vert1.position);
  vert1.uv[0] = 0;
  vert1.uv[1] = 1;

  struct Vertex vert2;
  init_vec3(-1.0F, -1.0F, 0.0, vert2.position);
  vert2.uv[0] = 0;
  vert2.uv[1] = 0;

  struct Vertex vert3;
  init_vec3(1.0, 1.0, 0.0, vert3.position);
  vert3.uv[0] = 1;
  vert3.uv[1] = 1;

  struct Vertex vert4;
  init_vec3(1.0, -1.0F, 0.0, vert4.position);
  vert4.uv[0] = 1;
  vert4.uv[1] = 0;

  array_init(&gui_vertex_array, sizeof(PVertex), 4);
  array_add(&gui_vertex_array, &vert1);
  array_add(&gui_vertex_array, &vert2);
  array_add(&gui_vertex_array, &vert3);
  array_add(&gui_vertex_array, &vert4);
}

void pe_gui_init() {

  array_init(&buttons, sizeof(Button), 20);

  init_gui_element_geometry();

  pe_gui_compile_shaders();

  create_vertex_buffer();

  pe_gui_compile_shaders();

  actual_buttons_array = &buttons;
}

void create_logo_shader() {
  logo_shader = create_engine_shader(pe_gui_shader_vert, pe_gui_shader_farg);
}

void draw_logo_image() {
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  Button load_screen_button;

  load_screen_button.relative_to = 400;
  init_button(&load_screen_button, camera_width_screen / 2,
              camera_heigth_screen / 2, 250, 250);

  Button *button = &load_screen_button;

  button->shader = logo_shader;

  glUseProgram(button->shader);
  glBindTexture(GL_TEXTURE_2D, logo_texture_id);

  two_dimension_screen_space_send_matrix(button->shader, button->size,
                                         button->position);

  glBindBuffer(GL_ARRAY_BUFFER, UI_plane_vertex_buffer_id);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(struct Vertex),
                        (void *)0);

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(struct Vertex),
                        (void *)offsetof(struct Vertex, uv));

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  check_error("logo");
}

void pe_gui_button_set_tex_with_path(Button *button, const char *path) {
  button->texture.format = GL_RGBA;
  int result = pe_load_image_with_format(path, GL_RGBA, &button->texture.image);
  if (result == -1)
    return;

  pe_tex_to_gpu(&button->texture);
  LOGW("button imgae");
}

void draw_logo() {

  PImage logo_image;
  int result = pe_load_image_with_format("../NativeContent/white_logo.png",
                                         GL_RGBA, &logo_image);
  if (result == -1)
    return;

  glGenTextures(1, &logo_texture_id);
  glBindTexture(GL_TEXTURE_2D, logo_texture_id);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, logo_image.width, logo_image.heigth,
               0, GL_RGBA, GL_UNSIGNED_BYTE, logo_image.pixels_data);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  free_image(&logo_image);

  init_gui_element_geometry();

  pe_gui_compile_shaders();

  create_vertex_buffer();

  create_logo_shader();

  draw_logo_image();
}

void draw_loading_screen() {
  glClearColor(1, 0.5, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT);
  draw_logo();
}

void update_user_iterface_status() {
  for (size_t i = 0; i < buttons.count; i++) {
    Button *button = array_get(&buttons, i);
    check_if_pressed(button);
    if (button->pressed) {
      ActionPointer *action_pointer =
          array_get(&actions_pointers, button->action_function_id);
      if (action_pointer->action == NULL) {
        LOG("No button action added\n");
        return;
      }
      action_pointer->action();
    }
  }
}

void draw_gui() {
  glCullFace(GL_FRONT);
  pe_buttons_draw();
  update_user_iterface_status();
  glCullFace(GL_BACK);
}

void new_empty_button() {
  Button new_button;
  ZERO(new_button);
  init_button(&new_button, camera_width_screen / 2, camera_heigth_screen / 2,
              15, 15);

  new_button.action_function_id = 0;
  new_button.pressed = false;

  PEShaderCreation creation;
  creation.pixel = pe_gui_shader_farg;
  creation.vertex = pe_gui_shader_vert;
  creation.shader = &new_button.shader;

  //pe_th_exec_in(pe_th_render_id, &pe_shader_new, &creation);
  pe_shader_new(&creation);

  //pe_th_wait(&thread_main);

  array_add(actual_buttons_array, &new_button);
}

void load_gui(const char *name) {
  char save_name[50];
  memset(save_name, 0, sizeof(save_name));
#ifdef EDITOR
  // strcat(save_name, gui_folder);
  strcat(save_name, name);
  strcat(save_name, ".gui");
#else
  strcpy(save_name, name);
#endif // DEBUG

  File level_file;
  if (load_file(save_name, &level_file) == -1) {
    LOG("GUI file not found: %s\n", name);
    return;
  }
  actual_buttons_array = &buttons;

  pe_parser_gui(level_file.data);

  close_file(&level_file);

  for (int i = 0; i < buttons.count; i++) {
    Button *button = array_get(&buttons, i);
    button->shader =
        create_engine_shader(pe_gui_shader_vert, pe_gui_shader_farg);
    button->pressed = false;
  }
}
