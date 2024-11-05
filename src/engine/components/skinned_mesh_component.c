#include "skinned_mesh_component.h"
#include <engine/engine.h>
void pe_comp_skinned_mesh_init(ComponentDefinition *element_component) {

  PSkinnedMeshComponent *mesh_component =
      element_component->data; // this is all ZERO
  if (mesh_component == NULL) {
    LOG("########## Skin mesh is NULL");
  }

  LOG("*********** SkinnedMesh array_models_loaded count %i",
      array_models_loaded.count);
  PModel *original_model = array_pop(&array_models_loaded);
  if (!original_model) {
    LOG("*******SKinned initialization error, original_model Model null");
  }

  new_empty_model();

  duplicate_model_data(selected_model, original_model);

  PEShaderCreation shader_creation;
  ZERO(shader_creation);
  shader_creation.vertex = shader_skin_vertex; // TODO:Android

  shader_creation.pixel = shader_skin_diffuse;
  shader_creation.shader = &selected_model->shader;

  pe_shader_new(&shader_creation);


  memcpy(mesh_component, pe_curr_skin_loading, sizeof(PSkinnedMeshComponent));

  glm_mat4_copy(element_component->parent->transform->model_matrix,
                selected_model->model_mat);

  TransformComponent *parent_transform = get_component_from_element(
      element_component->parent, TRASNFORM_COMPONENT);
  if (!parent_transform) {
    LOG("##### Skinned mesh, no parent transform");
  }

  mesh_component->transform = parent_transform;

  mesh_component->mesh = selected_model;

  // glm_mat4_copy(selected_model->model_mat,mesh_component->transform->model_matrix);

  if (original_model->vertex_array.count == 0) {
    LOG("***********SkinnedMesh initialization, orignal model vertex array "
        "equal 0");
  }
  mesh_component->mesh->mesh.index_array = original_model->index_array;
  mesh_component->mesh->mesh.vertex_array = original_model->vertex_array;
  mesh_component->mesh->mesh.vertex_buffer_id =
      original_model->vertex_buffer_id;
  mesh_component->mesh->mesh.index_buffer_id = original_model->index_buffer_id;

  mesh_component->mesh->index_array = original_model->index_array;
  mesh_component->mesh->vertex_array = original_model->vertex_array;
  mesh_component->mesh->vertex_buffer_id = original_model->vertex_buffer_id;
  mesh_component->mesh->index_buffer_id = original_model->index_buffer_id;

  pe_curr_skin_loading = NULL; // for new skin loading
  pe_anim_nodes_update(mesh_component);
}
