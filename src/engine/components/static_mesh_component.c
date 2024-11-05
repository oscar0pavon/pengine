#include "static_mesh_component.h"
#include <engine/engine.h>
#include "components.h"

#ifdef EDITOR
#include "../../editor/editor.h"
#endif

void pe_comp_static_mesh_update(ComponentDefinition *element_component) {

  StaticMeshComponent *mesh_component = element_component->data;

  if (mesh_component->meshes.initialized == true) {
    for (u8 i = 1; i <= mesh_component->meshes.count - 1; i++) {
      u8 *id = array_get(&mesh_component->meshes, i);
      PModel *model = array_get(actual_model_array, *id);

      glm_mat4_copy(element_component->parent->transform->model_matrix,
                    model->model_mat);
    }
    u8 *id = array_get(&mesh_component->meshes, 1);
    PModel *model = array_get(actual_model_array, *id);
    if (!model)
      return;
    glm_vec3_copy(model->min, mesh_component->bounding_box[0]);
    glm_vec3_copy(model->max, mesh_component->bounding_box[1]);
    glm_aabb_transform(mesh_component->bounding_box,
                       element_component->parent->transform->model_matrix,
                       mesh_component->bounding_box);
    glm_aabb_center(mesh_component->bounding_box, mesh_component->center);
  }

  PModel *modelf = array_get_pointer(&mesh_component->models_p, 0);

  glm_mat4_copy(element_component->parent->transform->model_matrix,
                modelf->model_mat);
}

void pe_shader_create_for_model(PModel* model, u32 frag, u32 vert) {

  // Shaders
  PEShaderCreation shader_creation;
  ZERO(shader_creation);
  shader_creation.model = model;
  shader_creation.vertex = vert;
  shader_creation.pixel = frag;


  pe_shader_create(&shader_creation);
}

void pe_comp_static_mesh_shader_init(PModel* model) {

  // Shaders
  PEShaderCreation shader_creation;
  ZERO(shader_creation);
  shader_creation.model = model;
  shader_creation.vertex = standart_vertex_shader;
  shader_creation.pixel = standart_fragment_shader;

  thread_main.wait = true;

  pe_shader_create(&shader_creation);

}

void pe_comp_static_mesh_texture_fill(StaticMeshComponent* mesh_component,int i){

    // Textures id
    u8 *texture_id = NULL;
    if (mesh_component->textures.count > 0)
      texture_id = array_get(&mesh_component->textures, i);

    if (texture_id) {
      PTexture *texture = array_get(current_textures_array, *texture_id);
      if (texture)
        selected_model->texture.id = texture->id;
    } else {
    #ifdef EDITOR
      selected_model->texture.id = editor_texture_checker.id;
    #endif
    }
}

void pe_comp_static_mesh_add_to_element() {

  actual_model_array = &array_models_loaded;
  new_empty_model();

  StaticMeshComponent mesh_component;
  ZERO(mesh_component);
  array_new_pointer(&mesh_component.models_p, 1);
  array_add_pointer(&mesh_component.models_p, selected_model);

  add_component_to_selected_element(sizeof(StaticMeshComponent),
                                    &mesh_component, STATIC_MESH_COMPONENT);
}

void pe_comp_static_mesh_init(ComponentDefinition *element_component) {

  StaticMeshComponent *mesh_component = element_component->data;

  for (int i = 0; i < mesh_component->models_p.count; i++) {

    PModel *model = array_get_pointer(&mesh_component->models_p, i);

    pe_comp_static_mesh_shader_init(model);

    glm_mat4_copy(element_component->parent->transform->model_matrix,
                  selected_model->model_mat);
  }

  if (mesh_component->meshes.initialized == true) {
    // fill meshes of StaticMeshComponent
    for (u32 i = 1; i <= mesh_component->meshes.count - 1; i++) {

      // Models ids
      u8 *id = array_get(&mesh_component->meshes, i);

      PModel *original_model = array_get(&array_models_loaded, *id);
      if (!original_model)
        return;


      pe_comp_static_mesh_texture_fill(mesh_component, i);

      glm_mat4_copy(element_component->parent->transform->model_matrix,
                    selected_model->model_mat);

      selected_model = original_model;
    }

    u8 id = actual_model_array->count - (mesh_component->meshes.count - 1);
    for (u8 i = 1; i <= mesh_component->meshes.count - 1; i++) {
      u8 *geted_id = array_get(&mesh_component->meshes, i);
      memcpy(geted_id, &id, sizeof(u8));
      id++;
    }
  }
  update_component(element_component);
}
