#include "components.h"
#include "../../engine/engine.h"

#include "../../engine/model.h"
#include "../animation/animation.h"

#include "skinned_mesh_component.h"
#include "static_mesh_component.h"

void add_component_to_selected_element(int component_byte_size,
                                       void *new_component,
                                       ComponentType type) {
  ComponentDefinition new_element_component;
  memset(&new_element_component, 0, sizeof(ComponentDefinition));
  new_element_component.type = type;
  new_element_component.id = components_id_count;
  new_element_component.parent = selected_element;
  new_element_component.bytes_size = component_byte_size;
  new_element_component.data =
      allocate_stack_memory_alignmed(component_byte_size, 16);
  memcpy(new_element_component.data, new_component, component_byte_size);

  array_add(&selected_element->components, &new_element_component);
}

void init_sphere_component(SphereComponent *component) {
  previous_models_array = actual_model_array;
  actual_model_array = &engine_native_models;
  new_empty_model();
  actual_model_array = previous_models_array;
  PModel *sphere_model = array_get(&engine_native_models, 0);
  memcpy(selected_model, sphere_model, sizeof(PModel));
  selected_model->shader_id =
      create_engine_shader(standart_vertex_shader, standart_fragment_shader);
  component->model = selected_model;
}

void init_cube_component(CubeComponent *component) {
  previous_models_array = actual_model_array;
  actual_model_array = &engine_native_models;
  new_empty_model();
  actual_model_array = previous_models_array;
  PModel *cube_model = array_get(&engine_native_models, 1);
  memcpy(selected_model, cube_model, sizeof(PModel));
  selected_model->shader_id =
      create_engine_shader(standart_vertex_shader, standart_fragment_shader);
  component->model = selected_model;
}

void init_camera_component(CameraComponent *component) {
  previous_models_array = actual_model_array;
  actual_model_array = &engine_native_models;
  new_empty_model();
  actual_model_array = previous_models_array;
  PModel *camera_model = array_get(&engine_native_models, 2);
  memcpy(selected_model, camera_model, sizeof(PModel));
  selected_model->shader_id =
      create_engine_shader(standart_vertex_shader, standart_fragment_shader);
  component->camera_gizmo = selected_model;

  glm_vec3_copy(VEC3(0, 0, 1), component->up),
      glm_vec3_copy((vec3){0, -1, 0}, component->front);
  glm_mat4_copy(main_camera.projection, component->projection);
  vec3 look_pos;
  glm_vec3_add(selected_element->transform->position, component->front,
               look_pos);
  glm_vec3_copy(VEC3(0, 0, 0), component->position);
  glm_lookat(selected_element->transform->position, look_pos, main_camera.up,
             component->view);
}

void change_view_to_camera_component(CameraComponent *camera_component) {
  memcpy(&saved_camera, &main_camera, sizeof(CameraComponent));

  memcpy(&main_camera, camera_component, sizeof(CameraComponent));
}

void update_main_camera_with_camera_component_values(
    CameraComponent *camera_component) {
  mat4 local;
  glm_mat4_identity(local);
  glm_translate(local, camera_component->position);

  mat4 global;
  glm_mat4_mul(selected_element->transform->model_matrix, local, global);
  vec3 global_position;
  glm_vec3_copy(VEC3(global[3][0], global[3][1], global[3][2]),
                main_camera.position);
  vec3 direction;
  vec3 direction_pivot;
  glm_vec3_add(selected_element->transform->position, VEC3(0, 0, 1.5),
               direction_pivot);
  glm_vec3_sub(direction_pivot, main_camera.position, direction);
  glm_vec3_normalize(direction);
  glm_vec3_copy(direction, main_camera.front);

  camera_update(camera_component);
}

void init_transfrom_component(TransformComponent *component) {
  memset(component->position, 0, sizeof(vec3));
  glm_vec3_copy(VEC3(1, 1, 1), component->scale);
  glm_mat4_identity(component->model_matrix);
  glm_quat_identity(component->rotation);
}

void clean_component_value(ComponentDefinition *component) {
  if (component->type == LEVEL_OF_DETAIL_COMPONENT) {
    LevelOfDetailComponent *details = component->data;
    if (details->hirarchical_level_of_detail) {
      details->hirarchical_level_of_detail->draw = false;
      details->hirarchical_level_of_detail->drew = false;
    }
  }
}

void update_component(ComponentDefinition *element_component) {
  switch (element_component->type) {
  case STATIC_MESH_COMPONENT: {
    pe_comp_static_mesh_update(element_component);
  } break;
  case COMPONENT_SKINNED_MESH: {
    PSkinnedMeshComponent *skin = element_component->data;
    glm_mat4_copy(element_component->parent->transform->model_matrix,
                  skin->mesh->model_mat);

  } break;
  case TRASNFORM_COMPONENT: {

    break;
  }

  case COMPONENT_HLOD_BOX: {
    HLODBoxComponent *mesh_component = element_component->data;
    glm_vec3_copy(VEC3(1, 1, 1), mesh_component->bounding_box[0]);
    glm_vec3_copy(VEC3(-1, -1, -1), mesh_component->bounding_box[1]);
    glm_aabb_transform(mesh_component->bounding_box,
                       element_component->parent->transform->model_matrix,
                       mesh_component->bounding_box);
    glm_aabb_center(mesh_component->bounding_box, mesh_component->center);
    break;
  }

  default:
    break;
  }
}

/*Fill model for test occlusion*/
/* Add to  array_static_meshes_pointers for test occlusion
array_static_meshes_pointers it cleaned every frame*/
void update_per_frame_component(ComponentDefinition *element_component) {
  switch (element_component->type) {
  case SPHERE_COMPONENT: {
    SphereComponent *component = element_component->data;
    glm_mat4_copy(element_component->parent->transform->model_matrix,
                  component->model->model_mat);
    array_add(&models_for_test_occlusion, &component->model);
    break;
  }
  case CUBE_COMPONENT: {
    CubeComponent *component = element_component->data;
    glm_mat4_copy(element_component->parent->transform->model_matrix,
                  component->model->model_mat);
    array_add(&models_for_test_occlusion, &component->model);
    break;
  }
  case CAMERA_COMPONENT: {
    CameraComponent *component = element_component->data;
    mat4 local;
    glm_mat4_identity(local);
    glm_translate(local, component->position);

    glm_mat4_mul(element_component->parent->transform->model_matrix, local,
                 component->camera_gizmo->model_mat);
    array_add(&models_for_test_occlusion, &component->camera_gizmo);

    break;
  }

  case STATIC_MESH_COMPONENT: {
    StaticMeshComponent *static_mesh_component = element_component->data;
    array_add(&array_static_meshes_pointers, &static_mesh_component);
    break;
  }

  case COMPONENT_SKINNED_MESH: {
    PSkinnedMeshComponent *skinned_mesh_component = element_component->data;
    array_add(&array_skinned_mesh_pointers, &skinned_mesh_component);
    break;
  }
  case COMPONENT_HLOD: {

    HLODComponent *hlod_component = element_component->data;

    float distance =
        glm_vec3_distance(main_camera.position, hlod_component->center);
    if (distance >= hlod_component->distance) {
      array_add(&models_for_test_occlusion, &hlod_component->model);
      for (int i = 0; i < hlod_component->childs.count; i++) {
        Element **ppElement = array_get(&hlod_component->childs, i);
        Element *element = ppElement[0];
        element->proccess = false;
      }
      return;
    }

    if (hlod_component->has_childs_HLOD) {

    } else {
      for (int i = 0; i < hlod_component->childs.count; i++) {
        Element **ppElement = array_get(&hlod_component->childs, i);
        Element *element = ppElement[0];
        element->proccess = true;
      }
    }

    break;
  }
  }
}

void *pe_comp_get(ComponentType type) {
  if (!selected_element)
    return NULL;

  for (int i = 0; i < selected_element->components.count; i++) {
    ComponentDefinition *component =
        array_get(&selected_element->components, i);
    if (component->type == type) {
      return component->data;
    }
  }

  return NULL;
}
void *get_component_from_element(Element *element, ComponentType type) {
  Element *previous_element = selected_element;
  selected_element = element;
  Element *result = pe_comp_get(type);
  if (result) {
    selected_element = previous_element;
    return result;
  }
  selected_element = previous_element;
  return NULL;
}

void add_transform_component_to_selected_element() {
  TransformComponent transform;
  init_transfrom_component(&transform);
  add_component_to_selected_element(sizeof(TransformComponent), &transform,
                                    TRASNFORM_COMPONENT);
  selected_element->transform = pe_comp_get(TRASNFORM_COMPONENT);
}

void add_camera_component_to_selected_element() {
  CameraComponent camera_component;
  init_camera_component(&camera_component);
  add_component_to_selected_element(sizeof(CameraComponent), &camera_component,
                                    CAMERA_COMPONENT);
}

void component_add_HLOD_to_select_element() {
  HLODComponent hlod_component;
  memset(&hlod_component, 0, sizeof(HLODComponent));
  add_component_to_selected_element(sizeof(HLODComponent), &hlod_component,
                                    COMPONENT_HLOD);
}

void for_each_element_components(void (*do_to)(ComponentDefinition *)) {
  if (!actual_elements_array) {
    LOG("No actual elements array\n");
    return;
  }

  for (int i = 0; i < actual_elements_array->count; i++) {
    Element *element = array_get(actual_elements_array, i);
    if (!element->proccess)
      continue;
    if (element->components.count > 0) {
      for (int o = 0; o < element->components.count; o++) {
        ComponentDefinition *component = array_get(&element->components, o);
        do_to(component);
      }
    }
  }
}

void for_each_element_components_in_array_of_pp(
    Array *array, void (*do_to)(ComponentDefinition *)) {
  for (int i = 0; i < array->count; i++) {
    Element **ppElement = array_get(array, i);
    Element *element = ppElement[0];
    if (!element->proccess)
      continue;
    if (element->components.count > 0) {
      for (int o = 0; o < element->components.count; o++) {
        ComponentDefinition *component = array_get(&element->components, o);
        do_to(component);
      }
    }
  }
}
void pe_element_comp_init() {

  for (int i = 0; i < selected_element->components.count; i++) {
    ComponentDefinition *component_definition =
        array_get(&selected_element->components, i);

    init_element_component(component_definition);
  }
}

void init_element_component(ComponentDefinition *element_component) {
  switch (element_component->type) {
  case STATIC_MESH_COMPONENT: {
    pe_comp_static_mesh_init(element_component);
    break;
  }
  case COMPONENT_SKINNED_MESH: {
    pe_comp_skinned_mesh_init(element_component);
    break;
  }
  case TRASNFORM_COMPONENT: {

    TransformComponent *transform = get_component_from_element(
        element_component->parent, TRASNFORM_COMPONENT);
    element_component->parent->transform = transform;

    break;
  }
  default:
    break;
  }
}

void pe_mesh_fill_models_ids(Array *meshes, Array *textures,
                             u32 models_loaded) {

  // the first element is the id of the model path in texts.array
  array_init(meshes, sizeof(u8), models_loaded + 1);
  array_init(textures, sizeof(u8), models_loaded + 1);

  u8 model_path_id = pe_arr_models_paths.count - 1;
  array_add(meshes, &model_path_id);

  // add model loaded id
  u8 id = 0;
  if (models_loaded > 1) {
    id = array_models_loaded.count - models_loaded;
    for (u8 i = 0; i < models_loaded; i++) {
      array_add(meshes, &id);
      id++;
    }
  } else {
    id = array_models_loaded.count - 1;
    array_add(meshes, &id);
    LOGW("one loaded model");
    LOG("Id for new model is: %i\n", id);
  }
}

void pe_comp_add(u32 models_loaded) {
  new_empty_element();

  add_transform_component_to_selected_element();

  switch (current_loaded_component_type) {
  case COMPONENT_SKINNED_MESH: {
    LOG("********* SkinnedMesh component adding... ");
    PSkinnedMeshComponent skin_mesh_component;
    ZERO(skin_mesh_component);
    add_component_to_selected_element(sizeof(PSkinnedMeshComponent),
                                      &skin_mesh_component,
                                      COMPONENT_SKINNED_MESH);

    break;
  }
  case STATIC_MESH_COMPONENT: {
    LOG("********* StaticMesh component adding... ");
    StaticMeshComponent mesh_component;
    ZERO(mesh_component);
    pe_mesh_fill_models_ids(&mesh_component.meshes, &mesh_component.textures,
                            models_loaded);

    array_new_pointer(&mesh_component.models_p, models_loaded);

    if (models_loaded == 1) {
      PModel *model = array_pop(&array_models_loaded);

      selected_model->mesh.index_array.count =
          selected_model->index_array.count;
      selected_model->mesh.index_buffer_id = selected_model->index_buffer_id;
      selected_model->mesh.vertex_buffer_id = selected_model->vertex_buffer_id;
      selected_model->mesh.vertex_array = selected_model->vertex_array;

      array_add_pointer(&mesh_component.models_p, model);
    } else {

      for (int i = 0; i < models_loaded; i++) {
        int id = pe_data_loader_models_loaded_count + i;
        PModel *model = array_get(&array_models_loaded, id);

        array_add_pointer(&mesh_component.models_p, model);
        PModel *model_from_models_p = array_pop(&mesh_component.models_p);
        model_from_models_p->mesh.index_array.count = model->index_array.count;
        model_from_models_p->index_buffer_id = model->index_buffer_id;

        model_from_models_p->vertex_buffer_id = model->vertex_buffer_id;
      }
      pe_data_loader_models_loaded_count =
          pe_data_loader_models_loaded_count + models_loaded;
    }

    add_component_to_selected_element(sizeof(StaticMeshComponent),
                                      &mesh_component, STATIC_MESH_COMPONENT);
    break;
  }
  }

  for (int i = 0; i < selected_element->components.count; i++) {
    ComponentDefinition *component_definition =
        array_get(&selected_element->components, i);

    init_element_component(component_definition);
  }
}
