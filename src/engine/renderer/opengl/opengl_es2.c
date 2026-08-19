#include "opengl_es2.h"
#include <GLES2/gl2.h>
#include <engine/camera.h>
#include <engine/components/components.h>
#include <engine/engine.h>
#include <engine/log.h>
#include <signal.h>

PEINLINE void mvp_error(const char *uniform_name) {
  LOG("Uniform not found: %s\n", uniform_name);
  //  debug_break();
}

PEINLINE void check_send_matrix_error(const char *message) {
  GLenum error = glGetError();
  if (error != GL_NO_ERROR) {
    LOG("[X] Send %s matrix error, Error %08x \n", message, error);
  }
}

PEINLINE void check_error(const char *message) {
  GLenum error = glGetError();
  if (error != GL_NO_ERROR) {
    LOG("%s, Error %08x \n", message, error);
  }
}

PEINLINE GLint get_uniform_location(GLuint shader, const char *name) {
  GLint uniform = glGetUniformLocation(shader, name);
  if (uniform == -1) {
    // mvp_error(name);
  }
  return uniform;
}

PEINLINE void send_color_to_shader(u32 shader_id, vec4 color) {
  GLint uniform_color = get_uniform_location(shader_id, "color");

  glUniform4fv(uniform_color, 1, color);
}

PEINLINE void render_clear_buffer(int buffer_bits) { glClear(buffer_bits); }

void init_static_gpu_vertex_buffer(Array *array, GLuint *id) {
  glGenBuffers(1, id);
  GLuint id_copy;
  memcpy(&id_copy, id, sizeof(GLuint));
  glBindBuffer(GL_ARRAY_BUFFER, id_copy);
  glBufferData(GL_ARRAY_BUFFER, array->count * sizeof(PVertex), array->data,
               GL_STATIC_DRAW);
}

void init_static_gpu_index_buffer(Array *array, GLuint *id) {
  glGenBuffers(1, id);
  GLuint id_copy;
  memcpy(&id_copy, id, sizeof(GLuint));
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id_copy);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, array->count * sizeof(PVertex),
               array->data, GL_STATIC_DRAW);
}

void update_gpu_vertex_data(Array *array, GLuint id) {
  glBindBuffer(GL_ARRAY_BUFFER, id);
  glBufferData(GL_ARRAY_BUFFER, array->count * sizeof(PVertex), array->data,
               GL_DYNAMIC_DRAW);
}

void GPU_buffers_create_for_model(PModel *model) {
  Array *vertex_array = &model->vertex_array;
  Array *index_array = &model->index_array;

  for (int i = 0; i < vertex_array->count; i++) {
    PVertex *vertex = array_get(vertex_array, i);

    //  LOG("######weight %f %f %f %f", vertex->weight[0], vertex->weight[1],
    //  vertex->weight[2], vertex->weight[3]);
    //       LOG("######weight %f %f %f %f", vertex->joint[0],
    //       vertex->weight[1], vertex->weight[2], vertex->weight[3]);
  }
  glGenBuffers(1, &model->vertex_buffer_id);
  glBindBuffer(GL_ARRAY_BUFFER, model->vertex_buffer_id);
  glBufferData(GL_ARRAY_BUFFER, vertex_array->count * sizeof(PVertex),
               vertex_array->data, GL_STATIC_DRAW);

  //INFO the index element size is whatever the gltf accessor used - u8, u16 or
  //u32 - so count * sizeof(u8) uploaded a quarter of a 32 bit index buffer and
  //half of the u16 one every chess piece has. bytes_size is what the array holds
  glGenBuffers(1, &model->index_buffer_id);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->index_buffer_id);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_array->bytes_size,
               index_array->data, GL_STATIC_DRAW);

  model->gpu_ready = true;
}

void pe_tex_to_gpu(PTexture *texture) {
  glGenTextures(1, &texture->id);
  glBindTexture(GL_TEXTURE_2D, texture->id);

  GLint format = 0;
  if (texture->format != GL_RGBA) {
    format = GL_RGB;
  } else {
    format = GL_RGBA;
  }

  // glTexImage2D(GL_TEXTURE_2D, 0, format, texture->width,
  //              texture->heigth, 0, format, GL_UNSIGNED_BYTE,
  //              texture->pixels_data);

  free_image(&texture->image);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glGenerateMipmap(GL_TEXTURE_2D);

  check_error("Texture to GPU");
  texture->gpu_loaded = true;
  thread_main.wait = false;
}

void pe_skinned_send_matrices(PSkinnedMeshComponent *skin, GLuint shader,
                              GLuint buffer, mat4 matrix) {

  glUseProgram(shader);

  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  int uv_id = 1;
  int normal_id = 4;
  int joint_id = 2;
  int weith_id = 3;
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(PVertex), (void *)0);

  glEnableVertexAttribArray(uv_id);
  glVertexAttribPointer(uv_id, 2, GL_FLOAT, GL_FALSE, sizeof(PVertex),
                        (void *)offsetof(PVertex, uv));

  glEnableVertexAttribArray(normal_id);

  glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, sizeof(PVertex),
                        (void *)offsetof(PVertex, normal));

  GLint model_uniform = get_uniform_location(shader, "model");
  GLint projection_uniform = get_uniform_location(shader, "projection");
  GLint view_uniform = get_uniform_location(shader, "view");

  glUniformMatrix4fv(model_uniform, 1, GL_FALSE, &matrix[0][0]);

  glUniformMatrix4fv(view_uniform, 1, GL_FALSE, &main_camera.view[0][0]);

  glUniformMatrix4fv(projection_uniform, 1, GL_FALSE,
                     &main_camera.projection[0][0]);

  GLint joints_matrices_uniform = get_uniform_location(shader, "joint_matrix");

  glEnableVertexAttribArray(joint_id);
  glVertexAttribPointer(joint_id, 4, GL_INT, GL_FALSE, sizeof(PVertex),
                        (void *)offsetof(PVertex, joint));
  glEnableVertexAttribArray(weith_id);
  glVertexAttribPointer(weith_id, 4, GL_FLOAT, GL_FALSE, sizeof(PVertex),
                        (void *)offsetof(PVertex, weight));

  glUniformMatrix4fv(joints_matrices_uniform, skin->node_uniform.joint_count,
                     GL_FALSE, skin->node_uniform.joints_matrix);
}
void pe_render_skinned_model(PSkinnedMeshComponent *skin) {
  PModel *new_model = skin->mesh;
  if (!new_model) {
    LOG("Not model in skinned mesh component");
    return;
  }

  glUseProgram(new_model->shader_id);
  /*
    if(new_model->texture_count == 2) {

      GLint uniform_texture0 = get_uniform_location(new_model->shader,
    "in_texture01"); GLint uniform_texture1 =
    get_uniform_location(new_model->shader, "in_texture02");

      GLint uniform_texture_id = get_uniform_location(new_model->shader,
    "texture_id");


      glUniform1i(uniform_texture_id, 0);

      glUniform1i(uniform_texture0, 0);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, new_model->textures[0].id);


      glUniform1i(uniform_texture_id, 1);
      glUniform1i(uniform_texture1, 1);
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, new_model->textures[1].id);



    }else{

      glBindTexture(GL_TEXTURE_2D, new_model->texture.id);
    }

  */

  glBindTexture(GL_TEXTURE_2D, new_model->texture.id);
  pe_skinned_send_matrices(skin, new_model->shader_id,
                           new_model->mesh.vertex_buffer_id,
                           new_model->model_mat);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, new_model->mesh.index_buffer_id);

  //  send_color_to_shader(new_model->shader, new_model->material.color);

  if (new_model->mesh.index_array.count == 0) {
    LOG("Index is equal to 0, skinned model not render\n");
  }

  glDrawElements(GL_TRIANGLES, new_model->mesh.index_array.count,
                 GL_UNSIGNED_SHORT, (void *)0);

  // check_error("sigle model error");
}

void update_draw_vertices(GLuint shader, GLuint buffer, mat4 matrix) {

  glUseProgram(shader);

  glBindBuffer(GL_ARRAY_BUFFER, buffer);

  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glEnableVertexAttribArray(2);
  glEnableVertexAttribArray(3);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(struct PVertex),
                        (void *)0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(struct PVertex),
                        (void *)offsetof(struct PVertex, uv));
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(struct PVertex),
                        (void *)offsetof(struct PVertex, color));
  glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(struct PVertex),
                        (void *)offsetof(struct PVertex, normal));

  GLint mvp_uniform = get_uniform_location(shader, "MVP");

  glUniformMatrix4fv(mvp_uniform, 1, GL_FALSE, &matrix[0][0]);
  // check_send_matrix_error("MVP");
}

void draw_vertices_like(GLenum mode, PModel *model, vec4 color) {
  update_draw_vertices(model->shader_id, model->vertex_buffer_id,
                       model->model_mat);
  // send_color_to_shader(model->shader,color);
  glDrawArrays(GL_POINTS, 0, model->vertex_array.count);
  check_error("simple draw");
}

void pe_render_2d(DrawData *data, vec2 position, vec2 size, vec4 color) {
  glDisable(GL_CULL_FACE);

  glUseProgram(data->shader);
  glBindTexture(GL_TEXTURE_2D, data->texture);

  two_dimension_screen_space_send_matrix(data->shader, size, position);

  glBindBuffer(GL_ARRAY_BUFFER, data->vertex);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(struct PVertex),
                        (void *)0);

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(struct PVertex),
                        (void *)offsetof(struct PVertex, uv));

  if (color != NULL) {
    send_color_to_shader(data->shader, color);
  }

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  check_error("Two dimension");
}

void draw_model_with_color(PModel *model, GLenum mode, vec4 color) {
  update_draw_vertices(model->shader_id, model->vertex_buffer_id,
                       model->model_mat);

  send_color_to_shader(model->shader_id, color);

  check_error("color matrix error");
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->index_buffer_id);

  glDrawElements(mode, model->index_array.count, GL_UNSIGNED_SHORT, (void *)0);
}

void draw_model_like(PModel *model, GLenum mode) {
  update_draw_vertices(model->shader_id, model->vertex_buffer_id,
                       model->model_mat);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->index_buffer_id);

  glDrawElements(mode, model->index_array.count, GL_UNSIGNED_SHORT, (void *)0);
}

void draw_simgle_model(PModel *new_model) {
  mat4 mvp;
  update_mvp(new_model->model_mat, mvp);

  update_draw_vertices(new_model->shader_id, new_model->mesh.vertex_buffer_id,
                       mvp);

  glBindBuffer(GL_ARRAY_BUFFER, new_model->mesh.vertex_buffer_id);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, new_model->mesh.index_buffer_id);

  GLint model_mat_uniform = get_uniform_location(new_model->shader_id, "uModel");

  check_send_matrix_error("Model Matrix");

  glUniformMatrix4fv(model_mat_uniform, 1, GL_FALSE,
                     &new_model->model_mat[0][0]);

  send_color_to_shader(new_model->shader_id, new_model->material.color);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, new_model->mesh.index_buffer_id);

  if (new_model->mesh.index_array.count == 0) {
    LOG("Draw "
        "simgle_model()"
        " error: Index is equal to 0, model not render\n");
    return;
  }

  glBindTexture(GL_TEXTURE_2D, new_model->texture.id);
  glDrawElements(GL_TRIANGLES, new_model->mesh.index_array.count,
                 GL_UNSIGNED_SHORT, (void *)0);

  check_error("sigle model error");
}
