
#include <engine/log.h>
#include <engine/utils.h>
#include <engine/vertex.h>

#include <engine/components/static_mesh_component.h>
#include <engine/engine.h>
#include <engine/renderer/opengl/opengl_es2.h>
#include <engine/renderer/renderer.h>

#include <engine/macros.h>

#include <engine/files.h>

GLuint new_mesh_vertex_buffer;

PModel model;

Array vertex_modeling;

uint current_select_vertex;

void pe_modeling_update_vertex_selected(PModel *model) {
  for (int i = 0; i < model->vertex_array.count; i++) {
    PVertex *vertex = array_get(&model->vertex_array, i);
    if (vertex->selected == true) {
      glm_vec3_copy(VEC3(0, 1, 0), vertex->color);
    } else {
      glm_vec3_copy(VEC3(0, 0, 1), vertex->color);
    }
  }
}

PVertex *pe_modeling_get_vertex_by_id(Array *array, u64 id) {
  PVertex *out = NULL;
  for (u64 i = 0; i < array->count; i++) {
    PVertex *vertex = array_get(array, i);
    if (vertex->id == id) {
      return vertex;
    }
  }
  LOG("## Vertex not found");
}
void pe_modeling_vertex_update() {

  glBindBuffer(GL_ARRAY_BUFFER, new_mesh_vertex_buffer);

  glBufferData(GL_ARRAY_BUFFER, model.vertex_array.count * sizeof(PVertex),
               model.vertex_array.data, GL_DYNAMIC_DRAW);
}

void pe_modeling_select_next_vertex() {
  current_select_vertex++;
  PVertex *vertex =
      pe_modeling_get_vertex_by_id(&model.vertex_array, current_select_vertex);
  vertex->selected = true;
  pe_modeling_update_vertex_selected(&model);
  pe_modeling_vertex_update();
  LOG("## new modeling udpated");
}

void draw_vertices() {

  mat4 model_mat;
  glm_mat4_identity(model_mat);
  mat4 mvp;
  update_mvp(model_mat, mvp);

  glUseProgram(model.shader);

  glBindBuffer(GL_ARRAY_BUFFER, new_mesh_vertex_buffer);

  glEnableVertexAttribArray(0);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(PVertex), (void *)0);

  glEnableVertexAttribArray(1);

  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(PVertex),
                        (void *)offsetof(PVertex, color));

  GLint mvp_uniform = get_uniform_location(model.shader, "MVP");

  send_color_to_shader(model.shader, VEC4(0, 0, 1, 1));

  glUniformMatrix4fv(mvp_uniform, 1, GL_FALSE, mvp);
  glDrawArrays(GL_POINTS, 0, model.vertex_array.count);
}

void pe_modeling_vertex_copy_data(PVertex *vertex, Array *array) {

  PVertex new_vertex;
  ZERO(new_vertex);

  memcpy(&new_vertex, vertex, sizeof(PVertex));

  array_add(&model.vertex_array, &new_vertex);
}

void pe_modeling_move_vertex(Array *array, PVertex *vertex) {}

void pe_modeling_extrude_vertex(Array *vertex_array, PVertex *vertex) {

  PVertex *get_vertex = array_get(&model.vertex_array, 0);
  pe_modeling_vertex_copy_data(get_vertex, &model.vertex_array);

  pe_th_exec_function(pe_th_render_id, pe_modeling_vertex_update);
}

void vertex_new(float x, float y, float z) {

  PVertex new_vertex;

  ZERO(new_vertex);

  new_vertex.position[0] = x;
  new_vertex.position[1] = y;
  new_vertex.position[2] = z;

  new_vertex.selected = false;

  array_add(&model.vertex_array, &new_vertex);

  pe_modeling_update_vertex_selected(&model);

  pe_th_exec_function(pe_th_render_id, pe_modeling_vertex_update);

  pe_th_wait(&thread_main);

  LOG("## Vertex Added");
}

void init_modeling() {
  ZERO(model);

  array_init(&model.vertex_array, sizeof(PVertex), 100);

  glGenBuffers(1, &new_mesh_vertex_buffer);

  // pe_comp_static_mesh_shader_init(&model);

  GLuint frag = pe_shader_load_src_and_create(
      file_vertex_modeling, 
      GL_FRAGMENT_SHADER);

  pe_shader_create_for_model(&model, frag, standart_vertex_shader);

  current_select_vertex = -1;

  // LOG("## Modeling init\n");
}
