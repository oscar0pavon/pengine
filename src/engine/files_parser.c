#include "files_parser.h"

#include <engine/engine.h>

#include "ThirdParty/parson.h"

unsigned int elements_count = 0;

int element_id = 0;

const char *actual_json_file;

ComponentType current_component_type;

/*Level Parser Start*/

void pe_parse_single_component_with_type_id(JSON_Object *object, int type) {
  switch (type) {

  case TRASNFORM_COMPONENT: {
    JSON_Array *position_array = json_object_get_array(object, "position");
    JSON_Array *rotation_array = json_object_get_array(object, "rotation");
    JSON_Array *scale_array = json_object_get_array(object, "scale");
    vec3 position;
    vec4 rotation;
    vec3 scale;

    for (int i = 0; i < 3; i++) {
      float n = (float)json_array_get_number(position_array, i);
      position[i] = n;
    }

    for (int i = 0; i < 3; i++) {
      float n = (float)json_array_get_number(scale_array, i);
      scale[i] = n;
    }

    for (int i = 0; i < 4; i++) {
      float n = (float)json_array_get_number(rotation_array, i);
      rotation[i] = n;
    }

    add_transform_component_to_selected_element();
    TransformComponent *transform = pe_comp_get(TRASNFORM_COMPONENT);
    glm_vec3_copy(position, transform->position);
    glm_vec4_copy(rotation, transform->rotation);
    glm_vec3_copy(scale, transform->scale);

    glm_translate(transform->model_matrix, transform->position);
    rotate_element(selected_element, transform->rotation);
    glm_scale(transform->model_matrix, transform->scale);

  } break;
  case COMPONENT_SKINNED_MESH: {
    PSkinnedMeshComponent skin_mesh_component;
    ZERO(skin_mesh_component);
    add_component_to_selected_element(sizeof(PSkinnedMeshComponent),
                                      &skin_mesh_component,
                                      COMPONENT_SKINNED_MESH);
    break;
  }
  case STATIC_MESH_COMPONENT: {
    StaticMeshComponent mesh_component;
    ZERO(mesh_component);
    add_component_to_selected_element(sizeof(StaticMeshComponent),
                                      &mesh_component, STATIC_MESH_COMPONENT);
    StaticMeshComponent *mesh = pe_comp_get(STATIC_MESH_COMPONENT);

    JSON_Array *models = json_object_get_array(object, "models");

    int models_ids_count = json_array_get_count(models);
    array_init(&mesh->meshes, sizeof(u8), models_ids_count);
    for (int i = 0; i < models_ids_count; i++) {
      u8 id = (u8)json_array_get_number(models, i);
      array_add(&mesh->meshes, &id);
    }

    JSON_Array *textures = json_object_get_array(object, "textures");

    int textures_ids_count = json_object_get_count(textures);
    array_init(&mesh->textures, sizeof(u8), textures_ids_count);
    for (int i = 0; i < textures_ids_count; i++) {
      u8 id = (u8)json_array_get_number(textures, i);
      array_add(&mesh->textures, &id);
    }

  } break;
  case CAMERA_COMPONENT: {
    add_camera_component_to_selected_element();
    break;
  }
  case PE_COMP_PLAYER_START: {

    PEComponentPlayerStart player_start_comp;
    ZERO(player_start_comp);
    add_component_to_selected_element(sizeof(PEComponentPlayerStart),
                                      &player_start_comp, PE_COMP_PLAYER_START);
    player_start = selected_element;
    break;
  }
  case SPHERE_COMPONENT: {
    SphereComponent sphere;
    ZERO(sphere);
    init_sphere_component(&sphere);
    add_component_to_selected_element(sizeof(SphereComponent), &sphere,
                                      SPHERE_COMPONENT);
    break;
  }
  default:
    break;
  }
}

void pe_parse_components(JSON_Array *array) {
  int count = json_array_get_count(array);
  for (int i = 0; i < count; i++) {
    JSON_Object *component_obj = json_array_get_object(array, i);
    int type = json_object_get_number(component_obj, "type");
    pe_parse_single_component_with_type_id(component_obj, type);
  }
}

int pe_parse_level(const char *json) {
  JSON_Value *level = json_parse_string(json);
  JSON_Array *level_array = json_object_get_array(json_object(level), "level");

  JSON_Object *elements_obj = json_array_get_object(level_array, 0);
  JSON_Object *data_ob = json_array_get_object(level_array, 1);

  JSON_Array *elements_array = json_object_get_array(elements_obj, "elements");

  int elements_count = json_array_get_count(elements_array);
  for (int i = 0; i < elements_count; i++) {
    JSON_Object *element_obj = json_array_get_object(elements_array, i);
    const char *name = json_object_get_string(element_obj, "name");

    new_empty_element();

    strcpy(selected_element->name, name);

    JSON_Array *component_array =
        json_object_get_array(element_obj, "components");
    pe_parse_components(component_array);

    element_id++;
  }

  JSON_Array *data_array = json_object_get_array(data_ob, "data");
  JSON_Object *models_paths_obj = json_array_get_object(data_array, 0);
  JSON_Object *textures_paths_obj = json_array_get_object(data_array, 1);

  JSON_Array *models_paths_array =
      json_object_get_array(models_paths_obj, "models");
  JSON_Array *textures_paths_array =
      json_object_get_array(textures_paths_obj, "textures");

  int textures_paths_count = json_array_get_count(textures_paths_array);
  int models_paths_count = json_array_get_count(models_paths_array);

  for (int i = 0; i < textures_paths_count; i++) {

    const char *path = json_array_get_string(textures_paths_array, i);
    array_add(&pe_arr_tex_paths, path);
  }

  for (int i = 0; i < models_paths_count; i++) {
    const char *path = json_array_get_string(models_paths_array, i);
    array_add(&pe_arr_models_paths, path);
  }
}

/*End Level Parser */

/*---------------------------------------------- */
/*UI file parser start */

void pe_parser_gui(const char *json) {
  JSON_Value *gui_val = json_parse_string(json);
  JSON_Array *buttons_arr =
      json_object_get_array(json_object(gui_val), "buttons");
  if (!buttons_arr) {
    LOGW("no array buttons");
  }
  int buttons_count = json_array_get_count(buttons_arr);
  // LOG("Buttons count: %i\n",buttons_count);
  for (int i = 0; i < buttons_count; i++) {
    new_empty_button();
    Button *new_button =
        array_get(actual_buttons_array, actual_buttons_array->count - 1);
    JSON_Object *button_json_obj = json_array_get_object(buttons_arr, i);
    strcpy(new_button->name, json_object_get_string(button_json_obj, "name"));
    JSON_Array *pos_arr = json_object_get_array(button_json_obj, "position");
    JSON_Array *size_arr = json_object_get_array(button_json_obj, "size");

    new_button->position[0] = (int)json_array_get_number(pos_arr, 0);
    new_button->position[1] = (int)json_array_get_number(pos_arr, 1);

    new_button->size[0] = (int)json_array_get_number(size_arr, 0);
    new_button->size[1] = (int)json_array_get_number(size_arr, 1);
  }
}

//*UI file parser end*/
