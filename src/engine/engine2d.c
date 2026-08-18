#include "engine2d.h"
#include "engine/array.h"
#include "engine/model.h"
#include "engine/numbers.h"
#include "engine/vertex.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <vulkan/vulkan_core.h>

#include "renderer/vk_buffer.h"
//#include "../window.h"
#include "renderer/uniform_buffer.h"
#include "renderer/descriptor_set.h"

#include <engine/utils.h>
#include "camera.h"

mat4 orthogonal_projection;

void pe_2d_init(){
  

  glm_ortho(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT, 0.1f, 1000.f, orthogonal_projection);

  
}

//pass the value UVs of character to an output of UV[4]
void pe_2d_get_character_uvs(UV*out_uvs, char character, float character_pixel_size, float texture_size){
    int ascii_value = (int)character;

    int char_x = ascii_value%16;
    int char_y = floor(ascii_value/16);

    float char_size_x = character_pixel_size/texture_size;
    float char_size_y = character_pixel_size/texture_size;


    UV uv1;
    UV uv2;
    UV uv3;
    UV uv4;

    //button left
    uv1.u = (float)char_x*char_size_x;
    uv1.v = (float)char_y*char_size_y;

    //button right
    uv2.u = (char_x+1)*char_size_x;
    uv2.v = char_y*char_size_y;


    //top right
    uv3.u = (char_x+1)*char_size_x;
    uv3.v = (char_y+1)*char_size_y;


    //top left
    uv4.u = char_x*char_size_x;
    uv4.v = (char_y+1)*char_size_y;

    out_uvs[0] = uv1;
    out_uvs[1] = uv4;
    out_uvs[2] = uv3;
    out_uvs[3] = uv2;
}

void pe_2d_draw(PModel* model, u32 image_index, vec2 position, vec2 size){

  mat4 model_matrix;
  glm_mat4_identity(model_matrix);

  glm_translate(model_matrix, (vec3){position[0], position[1], 0.0f});
  glm_scale(model_matrix, (vec3){size[0], size[1], 1.0f});

  glm_mat4_copy(model_matrix, model->uniform_buffer_object.model);

  pe_vk_send_uniform_buffer(model, image_index);

}

void pe_2d_create_character_geometry(Array *vertex_array, char character, int x,
                                     int y, int width, int height) {

  //Bottom left
  PVertex vert1;
  init_vec3(x, y, 0.5f, vert1.position);

  //top left
  PVertex vert2;
  init_vec3(x, y + height, 0.5f, vert2.position);

  //top right
  PVertex vert3;
  init_vec3(x + width, y + height, 0.5f, vert3.position);

  //bottom right
  PVertex vert4;
  init_vec3(x + width, y, 0.5f, vert4.position);

  
  UV character_uvs[4];
  pe_2d_get_character_uvs(character_uvs,character,32.f,512.f);

  vert1.uv[0] = character_uvs[0].u;
  vert1.uv[1] = character_uvs[0].v;

  vert2.uv[0] = character_uvs[1].u;
  vert2.uv[1] = character_uvs[1].v;

  vert3.uv[0] = character_uvs[2].u;
  vert3.uv[1] = character_uvs[2].v;

  vert4.uv[0] = character_uvs[3].u;
  vert4.uv[1] = character_uvs[3].v;
  
  array_add(vertex_array,&vert1);
  array_add(vertex_array,&vert2);
  array_add(vertex_array,&vert3);
  array_add(vertex_array,&vert4);
}
void generate_quad_indices(Array* index_array, int num_quads) {
    const uint32_t base_quad_indices[] = {0, 1, 2, 0, 2, 3};
    const int indices_per_quad = 6;
    const int vertices_per_quad = 4;

    for (int i = 0; i < num_quads; ++i) {
        // Calculate the base index for the current quad's vertices
        uint32_t vertex_offset = i * vertices_per_quad;

        // Apply the offset to the base quad pattern and store in the output array
        for (int j = 0; j < indices_per_quad; ++j) {
          u16 index = base_quad_indices[j] + vertex_offset;
          array_add(index_array, &index);
        }
    }
}

void pe_2d_create_text_geometry(PModel *model, const char *text, u8 size) {

  int char_count = 0;

  for (int i = 0; text[i] != '\0'; ++i) {
    char_count++;
  }
  array_init(&model->vertex_array, sizeof(PVertex),
             char_count * 4); // we have 4 vertice per character

  float current_x = 0;
  for (int i = 0; i < char_count; i++) {
    pe_2d_create_character_geometry(&model->vertex_array, text[i], current_x,
                                    0, size, size);
    current_x += 10; // kerner (space betwen characters)
  }


  array_init(&model->index_array, sizeof(u16), char_count*6);//we use 6 indices per character

  generate_quad_indices(&model->index_array, char_count);
  pe_2d_init_vulkan_buffers(model);
}

void pe_2d_init_vulkan_buffers(PModel* model){

  model->vertex_buffer = pe_vk_create_buffer(model->vertex_array.bytes_size,
                                             model->vertex_array.data,
                                             VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
  model->index_buffer = pe_vk_create_buffer(model->index_array.bytes_size,
                                            model->index_array.data,
                                            VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
  //init model matrix
  glm_mat4_identity(model->model_mat);
  //setup Uniform Buffer Object
  glm_mat4_copy(orthogonal_projection, model->uniform_buffer_object.projection);

  pe_vk_create_uniform_buffers(model);
  pe_vk_descriptor_pool_create(model);

  pe_vk_create_descriptor_sets(model, pe_vk_descriptor_set_layout_with_texture);
  //pe_vk_descriptor_with_image_update(model);//TODO
}

void pe_2d_create_quad_geometry(PModel* model){
  //Bottom left
  PVertex vert1;
  init_vec3(0, 0, 0.5f, vert1.position);
  vert1.uv[0] = 0;
  vert1.uv[1] = 0;

  //top left
  PVertex vert2;
  init_vec3(0, 1, 0.5f, vert2.position);
  vert2.uv[0] = 0;
  vert2.uv[1] = 1;

  //top right
  PVertex vert3;
  init_vec3(1, 1, 0.5f, vert3.position);
  vert3.uv[0] = 1;
  vert3.uv[1] = 1;

  //bottom right
  PVertex vert4;
  init_vec3(1, 0, 0.5f, vert4.position);
  vert4.uv[0] = 1;
  vert4.uv[1] = 0;


  array_init(&model->vertex_array,sizeof(PVertex),  4);//we have 4 vertices
  array_add(&model->vertex_array, &vert1);
  array_add(&model->vertex_array, &vert2);
  array_add(&model->vertex_array, &vert3);
  array_add(&model->vertex_array, &vert4);

  array_init(&model->index_array, sizeof(u16), 6);//we use 6 indices
  uint16_t number = 0;
  array_add(&model->index_array,&number);
  number = 1;
  array_add(&model->index_array,&number);
  number = 2;
  array_add(&model->index_array,&number);
  number = 0;
  array_add(&model->index_array,&number);
  number = 2;
  array_add(&model->index_array,&number);
  number = 3;
  array_add(&model->index_array,&number);

  pe_2d_init_vulkan_buffers(model);
}
