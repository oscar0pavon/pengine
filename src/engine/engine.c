//
// Created by pavon on 6/24/19.
//

#include "engine.h"
#include <unistd.h>

#include "game.h"
#include "images.h"

#include "model.h"

#include "LOD_system.h"

#include "elements.h"
#include "physics.h"
#include "window_manager.h"

#include <engine/animation/animation.h>

#include <editor/skeletal_editor.h>

#include <engine/renderer/render_thread.h>

#include <engine/base.h>

Array engine_elements;
Array engine_textures;

void pe_debug_print_mat4(mat4 mat) {

  LOG("matrix0 %f %f %f %f\n", mat[0][0], mat[0][1], mat[0][2], mat[0][3]);
  LOG("matrix1 %f %f %f %f\n", mat[1][0], mat[1][1], mat[1][2], mat[1][3]);
  LOG("matrix2 %f %f %f %f\n", mat[2][0], mat[2][1], mat[2][2], mat[2][3]);
  LOG("matrix3 %f %f %f %f\n", mat[3][0], mat[3][1], mat[3][2], mat[3][3]);
}

void pe_input_character(unsigned int codepoint) {
  if (codepoint == 241) // equal "ñ"
    return;
 
  char keyboard_utf = (char)codepoint;

#ifdef DESKTOP
  if (current_window->char_parser == NULL) {
    LOG("Not character parser assing to PEWindow\n");
    return;
  }
  current_window->char_parser(keyboard_utf);
#endif
#ifdef ANDROID
  pe_editor_parse_cmd_char(keyboard_utf);
#endif
}

void update_mvp(mat4 model, mat4 mvp_out) {
  mat4 projection_view;
  glm_mul(main_camera.projection, main_camera.view, projection_view);
  glm_mul(projection_view, model, mvp_out);
}

void pe_change_background_color(float r, float g, float b, float a) {

  vec4 color = {r, g, b, a};
  glm_vec4_copy(color, pe_background_color);
}

void select_last_element() {
  if (selected_element != NULL)
    selected_element->selected = false;
  selected_element = array_get_last(actual_elements_array);
  selected_element->selected = true;
}

void new_empty_element() {
  Element new_element;
  ZERO(new_element);

  new_element.id = element_id_count;
  new_element.proccess = true;

  element_id_count++;

  array_init(&new_element.components, sizeof(ComponentDefinition), 6);

  array_add(actual_elements_array, &new_element);

  select_last_element();
}

void new_empty_model_in_array(Array *array) {
  PModel new_model;
  memset(&new_model, 0, sizeof(PModel));
  array_add(array, &new_model);

  selected_model = array_get(array, array->count - 1);

  selected_model->id = array->count - 1;
}

void new_empty_model() {
  PModel new_model;
  ZERO(new_model);
  if (!actual_model_array)
    return;
  glm_mat4_identity(new_model.model_mat);
  array_add(actual_model_array, &new_model);

  selected_model = array_get(actual_model_array, actual_model_array->count - 1);

  selected_model->id = actual_model_array->count - 1;
}

void pe_mesh_data_fill_tex_ids(Array *meshes, Array *textures,
                               PTexture *texture, u8 path_id) {
  if (meshes->count >= 1) {
    array_add(textures, &path_id);
    for (u8 i = 1; i < meshes->count; i++) {
      u8 *model_id = array_get(meshes, i);
      PModel *model = array_get(actual_model_array, *model_id);
      model->texture.id = texture->id;
      array_add(textures, &path_id);
    }
  }
  LOG("###### pe_mesh_data_fill_tex_ids");
}

void pe_mesh_tex_fill_ids(PTexture *texture) {
  while (!texture->gpu_loaded) {
  }

  PSkinnedMeshComponent *skin_component = pe_comp_get(COMPONENT_SKINNED_MESH);
  if (!skin_component) {
    LOG("######## Not skin_componenet getted from pe_mesh_tex_fill_ids");
    return;
  }

  skin_component->mesh->textures[skin_component->mesh->texture_count] =
      *texture;

  skin_component->mesh->texture_count++;

  return;

  StaticMeshComponent *mesh = pe_comp_get(STATIC_MESH_COMPONENT);
  if (!mesh) {
    //				pe_mesh_data_fill_tex_ids(&mesh->meshes,&mesh->textures,texture,path_id);
    LOG("******* Texture added to StaticMeshComponent");
    return;
  }

  /*
      u8 id = pe_arr_tex_paths.count-1;
      Texture* last_texturer =
     array_get(current_textures_array,current_textures_array->count-1);
      skin_component->mesh->texture.id = last_texturer->id;

                  pe_mesh_data_fill_tex_ids(&skin_component->meshes,&skin_component->textures,texture,path_id);
  */
}
void pe_tex_loaded_to_model(int id) {
  PTexture *texture = array_get(current_textures_array, id);

  StaticMeshComponent *mesh = pe_comp_get(STATIC_MESH_COMPONENT);
  if (mesh) {
    pe_mesh_data_fill_tex_ids(&mesh->meshes, &mesh->textures, texture, id);
    return;
  }
}

void engine_add_texture_from_memory_to_selected_element(void *data, u32 size) {
  PTexture new_texture;
  texture_load_from_memory(&new_texture, size, data);
  array_add(&pe_arr_tex_paths, "from_memory");
  pe_mesh_tex_fill_ids(&new_texture);
}

void add_texture_to_selected_element_with_image_path(const char *image_path) {

  if (selected_element == NULL) {
    LOG("********No element selected\n");
    return;
  }
  if (image_path == NULL) {
    LOG("*****Error to load, null path (add_editor_texture - 154\n");
    return;
  }

  PTexture new_texture;
  ZERO(new_texture);

  if (!current_textures_array) {
    LOG("******* Not texture array");
    return;
  }
  array_add(current_textures_array, &new_texture);

  PTexture *texture_loaded = array_pop(current_textures_array);
  if (!texture_loaded) {
    LOG("******* texture_loaded is NULL from current_textures_array");
  }
  texture_loaded->gpu_loaded = false;
  if (pe_load_texture(image_path, texture_loaded) == -1) {

    LOG("******* texture_load() error");
    return;
  }

  array_add(&pe_arr_tex_paths, image_path);

  pe_mesh_tex_fill_ids(texture_loaded);

  LOG("Texture loaded and assigned to Mesh Component: %s\n", image_path);
}

void load_simple_image(const char *path) {

  PTexture new_texture;
  ZERO(new_texture);
  array_add(current_textures_array, &new_texture);

  PTexture *texture_loaded =
      array_get(current_textures_array, current_textures_array->count - 1);
  if (pe_load_texture(path, texture_loaded) == -1)
    return;
}

int add_element_with_model_path(const char *model_gltf_path) {
  if (model_gltf_path == NULL || model_gltf_path[0] == '\0') {
    LOG("Error to load, null path (add_editor_element)\n");
    return -1;
  }

  Array *prev_array = actual_model_array;
  actual_model_array = &array_models_loaded;

  int models_loaded = pe_loader_model(model_gltf_path);
  if (models_loaded == -1) {
    LOG("No model loaded from pe_loader_model()");
    return -1;
  } else {
    LOG("********* %i Models loaded", models_loaded);
  }
  if (prev_array != NULL)
    actual_model_array = prev_array;

  array_add(&pe_arr_models_paths, model_gltf_path); // needed for saved level

  pe_comp_add(models_loaded);
}

void set_element_position(Element *element, vec3 position) {
  glm_mat4_identity(element->transform->model_matrix);
  glm_translate(element->transform->model_matrix, position);
}

void add_action_function(void (*f)(void)) {
  ActionPointer new_action;
  new_action.id = action_pointer_id_count;
  new_action.action = f;
  array_add(&actions_pointers, &new_action);
  action_pointer_id_count++;
}

void load_model_to_array(Array *array, const char *path_model,
                         const char *color_texture_path) {

  Array *prev_model_array = actual_model_array;
  actual_model_array = array;

  pe_loader_model(path_model);

  selected_model->shader =
      create_engine_shader(standart_vertex_shader, standart_fragment_shader);

  glUseProgram(selected_model->shader);

  PTexture new_texture;
  pe_load_texture(color_texture_path, &new_texture);

  GPU_buffers_create_for_model(selected_model);

  selected_model->texture.id = new_texture.id;

  actual_model_array = prev_model_array;
}

void update_translation(vec3 translation) {
  TransformComponent *transform = pe_comp_get(TRASNFORM_COMPONENT);
  if (!transform)
    return;
  vec3 translation_per_frame;
  glm_vec3_scale(translation, time_delta, translation_per_frame);
  glm_translate(transform->model_matrix, translation_per_frame);
  glm_vec3_copy(transform->model_matrix[3], transform->position);

  for (int i = 0; i < selected_element->components.count; i++) {
    ComponentDefinition *component =
        array_get(&selected_element->components, i);
    update_component(component);
  }
  PSkinnedMeshComponent *skin = pe_comp_get(COMPONENT_SKINNED_MESH);
  if (!skin)
    return;

  pe_anim_nodes_update(skin);
}

void update_scale(vec3 translation) {
  TransformComponent *transform = pe_comp_get(TRASNFORM_COMPONENT);
  if (!transform)
    return;

  glm_vec3_add(transform->scale, translation, transform->scale);
  vec3 identity;
  glm_vec3_copy(GLM_VEC3_ONE, identity);
  glm_vec3_add(identity, translation, translation);

  glm_scale(transform->model_matrix, translation);

  for (int i = 0; i < selected_element->components.count; i++) {
    ComponentDefinition *component =
        array_get(&selected_element->components, i);
    update_component(component);
  }
}

void rotate_element(Element *element, versor quaternion) {
  mat4 model_rot_mat;
  glm_quat_mat4(quaternion, model_rot_mat);

  TransformComponent *transform =
      get_component_from_element(element, TRASNFORM_COMPONENT);
  if (transform)
    glm_mul(transform->model_matrix, model_rot_mat, transform->model_matrix);
}

void pe_element_rotate(Element *element, float angle, vec3 axis) {
  TransformComponent *transform =
      get_component_from_element(element, TRASNFORM_COMPONENT);
  if (transform == NULL) {
    LOG("No transfrom pointer in element\n");
    return;
  }
  versor new_rot_quat;
  glm_quatv(new_rot_quat, glm_rad(angle), axis);

  versor result_quat;
  glm_quat_mul(transform->rotation, new_rot_quat, result_quat);

  glm_quat_copy(result_quat, transform->rotation);

  mat4 model_rot_mat;
  glm_quat_mat4(new_rot_quat, model_rot_mat);

  glm_mul(transform->model_matrix, model_rot_mat, transform->model_matrix);

  for (int i = 0; i < selected_element->components.count; i++) {
    ComponentDefinition *component =
        array_get(&selected_element->components, i);
    update_component(component);
  }
}

void check_meshes_distance() {
  for (int i = 0; i < array_static_meshes_pointers_for_test_distance.count;
       i++) {
    StaticMeshComponent **ppStaticMesh =
        array_get(&array_static_meshes_pointers_for_test_distance, i);
    StaticMeshComponent *mesh_component = ppStaticMesh[0];

    /*Simple LOD */
    LOD_check_distance_static_mesh_component_and_add_to_draw_elements(
        mesh_component);
  }

  for (int i = 0; i < array_skinned_mesh_for_distance_test.count; i++) {
    PSkinnedMeshComponent **ppSkinComponent =
        array_get(&array_skinned_mesh_for_distance_test, i);
    array_add(&frame_draw_skinned_elements, &ppSkinComponent[0]->mesh);
  }
}

void test_elements_occlusion() {
  vec4 frustrum_planes[6];
  mat4 view_projection_mat;
  glm_mat4_mul(main_camera.projection, main_camera.view, view_projection_mat);
  glm_frustum_planes(view_projection_mat, frustrum_planes);

  // this is for gizmos and native elements
  for (size_t i = 0; i < models_for_test_occlusion.count; i++) {
    PModel **model = array_get(&models_for_test_occlusion, i);
    PModel *test_model = model[0];

    vec3 box[2];
    glm_vec3_copy(test_model->min, box[0]);
    glm_vec3_copy(test_model->max, box[1]);

    glm_aabb_transform(box, test_model->model_mat, box);

    if (glm_aabb_frustum(box, frustrum_planes) == true) {
      array_add(&frame_draw_static_elements, &model[0]);
    }
  }

  for (int i = 0; i < array_static_meshes_pointers.count; i++) {
    StaticMeshComponent **static_mesh_component =
        array_get(&array_static_meshes_pointers, i);
    if (glm_aabb_frustum(static_mesh_component[0]->bounding_box,
                         frustrum_planes) == true)
      array_add(&array_static_meshes_pointers_for_test_distance,
                &static_mesh_component[0]);
  }

  for (int i = 0; i < array_skinned_mesh_pointers.count; i++) {
    PSkinnedMeshComponent **skin_component =
        array_get(&array_skinned_mesh_pointers, i);
    // if(glm_aabb_frustum(skin_component[0]->bounding_box,frustrum_planes) ==
    // true)//TODO: skinned mesh culling
    array_add(&array_skinned_mesh_for_distance_test, &skin_component[0]);
  }
}

void duplicate_model_data(PModel *destination, PModel *source) {
  memcpy(destination, source, sizeof(PModel));
}

void pe_init_arrays() {
  actual_elements_array = &engine_elements;

  array_init(&engine_elements, sizeof(Element), 100);
  array_init(&pe_arr_models_paths, sizeof(char[100]), 50);
  array_init(&pe_arr_tex_paths, sizeof(char[20]), 50);

  array_init(&array_models_loaded, sizeof(PModel), 100);
  array_init(&pe_arr_skin_loaded, sizeof(PSkinnedMeshComponent), 100);

  array_init(&engine_native_models, sizeof(PModel), 100);

  array_init(&array_hirarchical_level_of_detail,
             sizeof(HierarchicalLevelOfDetail), 5);

  array_init(&actions_pointers, sizeof(ActionPointer), 20);

  array_init(&frame_draw_static_elements, sizeof(void *), 100);
  array_init(&frame_draw_skinned_elements, sizeof(void *), 100);

  array_init(&models_for_test_occlusion, sizeof(void *), 300);
  array_init(&array_static_meshes_pointers, sizeof(void *), 300);
  array_init(&array_static_meshes_pointers_for_test_distance, sizeof(void *),
             100);
  array_init(&array_skinned_mesh_for_distance_test, sizeof(void *), 100);
  array_init(&array_skinned_mesh_pointers, sizeof(void *), 100);

  array_init(&array_animation_play_list, sizeof(PEAnimationPlay), 100);

  array_init(&array_render_thread_init_commmands, sizeof(ExecuteCommand), 5);

  array_init(&array_render_thread_commands, sizeof(ExecuteCommand), 100);

  array_init(&render_thread_commads, sizeof(PEThreadCommand), 100);

  array_init(&engine_windows, sizeof(EngineWindow), 40);

  array_init(&pe_array_textures, sizeof(PTexture), 100);

  current_textures_array = &pe_array_textures;

  touch_position_x = -1;
  touch_position_x = -1;

  action_pointer_id_count = 0;

  actual_standard_fragment_shader = standart_fragment_shader;

  pe_is_window_init = false;

  pe_is_window_terminate = false;
}

void pe_init_global_variables() {

  pe_data_loader_models_loaded_count = 0;

  engine_running = true;

#ifdef DEBUG
  pe_bool_can_draw_skeletal = false;
  update_vertex_bones_gizmos = false;
#endif
}


void pe_program_main_loop(void (*program_loop)(void),
                          EngineWindow *program_window) {

  pe_init();

  EngineWindow win;
  ZERO(win);

  array_add(&engine_windows, &win);
  game_window = array_pop(&engine_windows);

  LOG("########## PE renderizer GO");
  pe_game_create_window();

  pe_game_render_init(); // editor init
  pe_game_render_config();
  // pe_render_thread_start_and_draw();
  render_thread_init();

  //*********  Timing ******
  float render_frame_time = 0;
  float disired_frame_time = 0.016f;

  u8 frames = 0;
  float frame_second = 0;

  while (!engine_initialized) {
  } // wait for initilization

#ifdef ANDROID
  pe_wm_check(program_window);
#endif

  //Main loop 
  while (!pe_wm_should_close(game_window)) {
    time_start();
    
    pe_wm_input_update();
    pe_game_input();
    if (pe_renderer_type == PEWMOPENGLES2) {
      pe_wm_context_current();
    }
    pe_wm_windows_draw();
    render_frame_time += time_delta;


    pe_game_draw();
    program_loop();

    if (pe_renderer_type == PEWMOPENGLES2) {
      pe_wm_swap_buffers();
    }
    pe_wm_events_update();
    
    //********* Timing **********
    time_end();

    frame_second += time_elapsed_time;

    if (frame_second >= 1000) {
      FPS = frames * (1000.f / frame_second);
      frames = 0;
      frame_second = 0;
    } else {
      frames++;
    }
    //********* End timing ********
  }
}
