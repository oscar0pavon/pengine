#include "gizmos.h"
#include "../engine/shader.h"

#include "HLOD_factory.h"
#include "geometry.h"
#include "skeletal_editor.h"
#include <editor/editor_mode.h>
#include <engine/renderer/renderer.h>
#include <engine/files.h>

Array debug_objects;
Array bounding_boxes;

vec3 last_position;

bool bounding_box_initialized = false;

bool bounding_sphere_initialized = false;

bool can_draw_box = true;

Grid new_grid;

void add_debug_line(vec3 start, vec3 end) {
  DebugLine new_line;
  memset(&new_line, 0, sizeof(DebugLine));
  array_add(&debug_objects, &new_line);
  DebugLine *pnew_line = array_get(&debug_objects, debug_objects.count - 1);
  glm_vec3_copy(start, pnew_line->start);
  glm_vec3_copy(end, pnew_line->end);
}

int create_bounding_vertices() {
  float *bounding_box = NULL;
  StaticMeshComponent *mesh_component = pe_comp_get(STATIC_MESH_COMPONENT);
  if (mesh_component) {
    bounding_box = mesh_component->bounding_box[0];
  }
  if (!mesh_component) {

    HLODBoxComponent *HLOD_box = pe_comp_get(COMPONENT_HLOD_BOX);
    if (!HLOD_box) {
      LOG("No valid component for draw bounding box\n");
      can_draw_box = false;
      return -1;
    } else {
      can_draw_box = true;
    }
    bounding_box = HLOD_box->bounding_box[0];
  }
  geometry_cube_create_vertices(bounding_box);
  return 0;
}

void init_selected_object_bounding_box_vertices() {
  if (bounding_box_initialized == false && selected_element != NULL) {
    Array *prev_model_array = actual_model_array;
    actual_model_array = &bounding_boxes;

    new_empty_model();

    selected_model->id = actual_model_array->count - 1;

    geometry_cube_create_indices();

    if (create_bounding_vertices() == -1) {
      can_draw_box = false;
      return;
    } else {
      can_draw_box = true;
    }

    init_static_gpu_vertex_buffer(&selected_model->vertex_array,
                                  &selected_model->vertex_buffer_id);
    init_static_gpu_index_buffer(&selected_model->index_array,
                                 &selected_model->index_buffer_id);

    selected_model->shader = create_engine_shader(
        standart_vertex_shader, shader_source_color_fragment_shader);
    bounding_box_initialized = true;
    actual_model_array = prev_model_array;
  }
}
void update_bounding_vertices_array(PModel *model) {

  glm_vec3_copy(selected_element->transform->position, last_position);

  PModel *box = array_get(&bounding_boxes, 0);

  array_clean(&box->vertex_array);
  selected_model = box;

  create_bounding_vertices();
}

void draw_bounding_box() {
  if (bounding_box_initialized == true) {

    PModel *bounding_model = array_get(&bounding_boxes, 0);

    update_bounding_vertices_array(bounding_model);
    if (can_draw_box == false)
      return;
    update_gpu_vertex_data(&bounding_model->vertex_array,
                           bounding_model->vertex_buffer_id);

    draw_model_with_color(bounding_model, GL_LINES, (vec4){0, 1, 0.2, 1});
    glDrawArrays(GL_POINTS, 0, bounding_model->vertex_array.count);
    return;
  }
  init_selected_object_bounding_box_vertices();
}

void init_line_vertices(DebugLine *line) {
  array_init(&line->vertex_array, sizeof(PVertex), 2);

  struct Vertex vert = {{line->start[0], line->start[1], line->start[2]},
                        {0, 0}};
  struct Vertex vert2 = {{line->end[0], line->end[1], line->end[2]}, {0, 0}};

  array_add(&line->vertex_array, &vert);
  array_add(&line->vertex_array, &vert2);

  init_static_gpu_vertex_buffer(&line->vertex_array, &line->vertex_buffer_id);
}

void update_line_vertices(DebugLine *line) {
  struct Vertex vert = {{line->start[0], line->start[1], line->start[2]},
                        {0, 0}};
  struct Vertex vert2 = {{line->end[0], line->end[1], line->end[2]}, {0, 0}};

  PVertex *vertices = array_get(&line->vertex_array, 0);
  memcpy(&vertices[0], &vert, sizeof(struct Vertex));
  memcpy(&vertices[1], &vert2, sizeof(struct Vertex));
}

void init_line(DebugLine *line) {
  init_line_vertices(line);
  line->shader = create_engine_shader(standart_vertex_shader,
                                      shader_source_color_fragment_shader);
  glm_vec4_copy((vec4){1, 1, 1, 1}, line->color);
}

void draw_axis_lines() {
  for (int i = 0; i < debug_objects.count; i++) {
    DebugLine *line = array_get(&debug_objects, i);
    if (line->initialized == true) {
      mat4 model;
      glm_mat4_identity(model);
      mat4 mvp;
      update_mvp(model, mvp);
      update_draw_vertices(line->shader, line->vertex_buffer_id, mvp);
      glLineWidth(1);
      GLint uniform_color = glGetUniformLocation(line->shader, "color");

      glUniform4fv(uniform_color, 1, line->color);
      GLenum error;
      error = glGetError();
      if (error != GL_NO_ERROR) {
        //        LOG("[X] Send uniform error, Error %08x \n", error);
      }
      glDrawArrays(GL_LINES, 0, line->vertex_array.count);
      continue;
    }
    init_line(line);
    line->initialized = true;
  }
  DebugLine *line;
  line = array_get(&debug_objects, 0);
  glm_vec4_copy((vec4){0, 0, 1, 1}, line->color);

  line = array_get(&debug_objects, 2);
  glm_vec4_copy((vec4){1, 0, 0, 1}, line->color);

  line = array_get(&debug_objects, 1);
  glm_vec4_copy((vec4){0, 1, 0, 1}, line->color);
}
void draw_camera_direction() {
  if (selected_element != NULL) {
    vec3 direction;
    CameraComponent *camera = array_get(&components, 0);
    vec3 look_pos;
    glm_vec3_add(selected_element->transform->position, camera->front,
                 look_pos);
    glm_vec3_sub(look_pos, selected_element->transform->position, direction);

    DebugLine *line = array_get(&debug_objects, 2);
    glm_vec3_copy(selected_element->transform->position, line->start);

    vec3 front_dir;
    glm_vec3_copy((vec3){0, -1, 0}, front_dir);

    vec3 direction_len;
    glm_vec3_mul((vec3){2, 2, 2}, front_dir, direction_len);

    vec3 pos2;
    glm_vec3_add(selected_element->transform->position, direction_len, pos2);

    glm_vec3_copy(pos2, line->end);
    update_line_vertices(line);
    update_gpu_vertex_data(&line->vertex_array, line->vertex_buffer_id);
  }
}

void init_grid_greometry() {
  array_init(&new_grid.vertex_array, sizeof(PVertex), 1000);

  for (int i = 1; i < 10; i++) {
    for (int o = 1; o < 10; o++) {
      PVertex vertex1;
      PVertex vertex2;
      PVertex vertex3;
      PVertex vertex4;
      PVertex vertex5;
      PVertex vertex6;
      PVertex vertex7;
      PVertex vertex8;

      glm_vec3_copy((vec3){i, o, 0}, vertex1.position);
      glm_vec3_copy((vec3){-i, o, 0}, vertex2.position);

      glm_vec3_copy((vec3){o, i, 0}, vertex3.position);
      glm_vec3_copy((vec3){o, -i, 0}, vertex4.position);

      glm_vec3_copy((vec3){i, -o, 0}, vertex5.position);
      glm_vec3_copy((vec3){-i, -o, 0}, vertex6.position);

      glm_vec3_copy((vec3){-o, i, 0}, vertex7.position);
      glm_vec3_copy((vec3){-o, -i, 0}, vertex8.position);

      array_add(&new_grid.vertex_array, &vertex1);
      array_add(&new_grid.vertex_array, &vertex2);
      array_add(&new_grid.vertex_array, &vertex3);
      array_add(&new_grid.vertex_array, &vertex4);
      array_add(&new_grid.vertex_array, &vertex5);
      array_add(&new_grid.vertex_array, &vertex6);
      array_add(&new_grid.vertex_array, &vertex7);
      array_add(&new_grid.vertex_array, &vertex8);
    }
  }

  init_static_gpu_vertex_buffer(&new_grid.vertex_array,
                                &new_grid.vertex_buffer_id);
  new_grid.shader = create_engine_shader(standart_vertex_shader,
                                         shader_source_color_fragment_shader);
}

void draw_grid() {
  mat4 model;
  glm_mat4_identity(model);
  mat4 mvp;
  update_mvp(model, mvp);
  update_draw_vertices(new_grid.shader, new_grid.vertex_buffer_id, mvp);
  glLineWidth(1);
  GLint uniform_color = glGetUniformLocation(new_grid.shader, "color");
  vec4 color = {0, 0, 0, 1};
  glUniform4fv(uniform_color, 1, color);

  glDrawArrays(GL_LINES, 0, new_grid.vertex_array.count);
}

void gizmos_boanding_sphere_draw(Sphere *sphere, vec4 color) {
  if (bounding_sphere_initialized) {
    int bounding_id = 0;
    if (bounding_boxes.count > 1) {
      bounding_id = 1;
    }
    PModel *bounding_model = array_get(&bounding_boxes, bounding_id);

    mat4 new_model_mat;
    glm_mat4_identity(new_model_mat);
    glm_translate(new_model_mat, sphere->center);
    glm_scale_uni(new_model_mat, sphere->radius * 0.5);
    update_draw_vertices(bounding_model->shader,
                         bounding_model->vertex_buffer_id, new_model_mat);

    GLint uniform_color = get_uniform_location(bounding_model->shader, "color");

    glUniform4fv(uniform_color, 1, color);
    check_error("color matrix error");

    glDrawArrays(GL_LINE_STRIP, 0, bounding_model->vertex_array.count);
    return;
  }

  Array *prev_model_array = actual_model_array;
  actual_model_array = &bounding_boxes;

  new_empty_model();

  selected_model->id = actual_model_array->count - 1;

  // create vertex for sphere
  geometry_sphere_create_vertices(32);

  init_static_gpu_vertex_buffer(&selected_model->vertex_array,
                                &selected_model->vertex_buffer_id);
  init_static_gpu_index_buffer(&selected_model->index_array,
                               &selected_model->index_buffer_id);

  selected_model->shader = create_engine_shader(
      standart_vertex_shader, shader_source_color_fragment_shader);
  bounding_sphere_initialized = true;
  actual_model_array = prev_model_array;
}

void gizmos_init() {
  array_init(&gizmos, sizeof(PModel), 10);

  array_init(&bounding_boxes, sizeof(PModel), 10);

  array_init(&debug_objects, sizeof(DebugLine), 300);

  load_model_to_array(
      &gizmos, file_translate_glb,
      file_transform_jpg);
  selected_model->mesh.vertex_buffer_id = selected_model->vertex_buffer_id;
  selected_model->mesh.index_buffer_id = selected_model->index_buffer_id;

  selected_model->mesh.vertex_array.count = selected_model->vertex_array.count;
  selected_model->mesh.index_array.count = selected_model->index_array.count;

  load_model_to_array(&gizmos,
                      file_rotate_glb,
                      file_rotate_png);
  load_model_to_array(
      &gizmos, file_scale_glb,
      file_transform_jpg);
  load_model_to_array(&gizmos,
                      file_camera_gltf,
                      file_camera_jpg);
  load_model_to_array(
      &gizmos, file_player_start_glb,
      file_player_start_jpg);

  can_draw_gizmos = true;
  can_draw_skeletal_bones = false;
  can_draw_bounding_box_in_select_element = false;

  draw_translate_gizmo = false;
  draw_rotate_gizmo = false;

  add_debug_line((vec3){0, 0, 5}, (vec3){0, 0, -5});
  add_debug_line((vec3){0, 5, 0}, (vec3){0, -5, 0});
  add_debug_line((vec3){5, 0, 0}, (vec3){-5, 0, 0});

  init_grid_greometry();
}

inline static void gizmos_update_transform(mat4 in, mat4 out) {
  glm_mat4_copy(in, out);
  glm_scale(out, VEC3(1, 1, 1));
}

void draw_gizmos() {

  draw_axis_lines();

  draw_grid();

  glClear(GL_DEPTH_BUFFER_BIT);

  if (can_draw_skeletal_bones)
    draw_skeletal_bones();

  if (can_draw_gizmos) {
    if (can_draw_bounding_box_in_select_element) {
      draw_bounding_box();
    }

    // draw_camera_direction();

    if (key_released(&input.KEY_4))
      gizmos_can_draw_spheres = true;

    if (gizmos_can_draw_spheres) {

      HLODs_generated_debug();
      if (selected_element) {
        StaticMeshComponent *mesh = pe_comp_get(STATIC_MESH_COMPONENT);
        if (!mesh)
          return;
        Sphere sphere;
        sphere.radius = glm_aabb_radius(mesh->bounding_box);
        glm_vec3_copy(mesh->center, sphere.center);
        gizmos_boanding_sphere_draw(&sphere, (vec4){0, 0, 1, 1});
      }
    }

    glClear(GL_DEPTH_BUFFER_BIT);

    PModel *actual_gizmo = NULL;

    if (draw_translate_gizmo) {
      actual_gizmo = array_get(&gizmos, 0);
    }
    if (draw_rotate_gizmo) {
      actual_gizmo = array_get(&gizmos, 1);
    }
    if (gizmos_draw_scale) {
      actual_gizmo = array_get(&gizmos, 2);
    }

    // Gizmo scale relative main camera position
    if (selected_element != NULL) {
      TransformComponent *transform = pe_comp_get(TRASNFORM_COMPONENT);
      if (transform && actual_gizmo) {
        glm_mat4_copy(transform->model_matrix, actual_gizmo->model_mat);
        float distance =
            glm_vec3_distance(main_camera.position, actual_gizmo->model_mat[3]);
        distance *= 0.2;
        glm_scale(actual_gizmo->model_mat, VEC3(distance, distance, distance));
      }
    }

    if (actual_gizmo != NULL)
      draw_simgle_model(actual_gizmo);

    PEComponentPlayerStart *player_start_comp =
        get_component_from_element(player_start, PE_COMP_PLAYER_START);
    if (player_start_comp) {
      PModel *player_start_gizmo_model = array_get(&gizmos, 4);

      TransformComponent *transform =
          get_component_from_element(player_start, TRASNFORM_COMPONENT);

      if (transform && player_start_gizmo_model) {
        glm_mat4_copy(transform->model_matrix,
                      player_start_gizmo_model->model_mat);
      }
      draw_simgle_model(player_start_gizmo_model);
    }

    // Draw camera gizmo
    if (editor_mode == EDITOR_DEFAULT_MODE) {
      if (current_component_selected) {
        CameraComponent *camera = current_component_selected->data;
        if (!camera)
          return;
        glm_mat4_copy(camera->camera_gizmo->model_mat, actual_gizmo->model_mat);
        float distance =
            glm_vec3_distance(main_camera.position, actual_gizmo->model_mat[3]);
        distance *= 0.008;
        glm_scale(actual_gizmo->model_mat, VEC3(distance, distance, distance));
        draw_simgle_model(actual_gizmo);
      }
      if (editor_mode == EDITOR_MODE_GUI_EDITOR) {
        if (selected_button == NULL)
          return;
        glUseProgram(actual_gizmo->shader);
        two_dimension_screen_space_send_matrix(actual_gizmo->shader,
                                               selected_button->size,
                                               selected_button->position);
        glBindTexture(GL_TEXTURE_2D, actual_gizmo->texture.id);
        glBindBuffer(GL_ARRAY_BUFFER, actual_gizmo->vertex_buffer_id);

        glEnableVertexAttribArray(0);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(struct Vertex),
                              (void *)0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, actual_gizmo->index_buffer_id);

        glDrawElements(GL_TRIANGLES, actual_gizmo->index_array.count,
                       GL_UNSIGNED_SHORT, (void *)0);
      }
    }
  }
}
