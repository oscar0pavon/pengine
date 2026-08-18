#include "model.h"
#include "engine/array.h"
#include "renderer/vulkan.h"
#include <vulkan/vulkan_core.h>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include "stdio.h"

#include <cglm/vec3.h>
#include "file_loader.h"

#include "engine/camera.h"

#include "vertex.h"

#include "renderer/vk_vertex.h"
#include "renderer/descriptor_set.h"
#include "renderer/uniform_buffer.h"

cgltf_data *current_data;


void pe_loader_mesh_read_accessor_indices(Array *index_array,
                                          cgltf_accessor *accessor) {
  switch (accessor->component_type) {
  case cgltf_component_type_r_8:
    break;
  case cgltf_component_type_r_16:
    break;
  case cgltf_component_type_r_8u:

    array_init(index_array, sizeof(u8), accessor->count);
    break;
  case cgltf_component_type_r_16u:

    array_init(index_array, sizeof(unsigned short), accessor->count);
    break;
  case cgltf_component_type_r_32f:
    break;
  case cgltf_component_type_r_32u:
    array_init(index_array, sizeof(unsigned int), accessor->count);
    break;
  }
  for (size_t i = 0; i < accessor->count; i++) {
    size_t index = cgltf_accessor_read_index(accessor, i);
    array_add(index_array, &index);
  }
}

void pe_loader_read_accessor(Array* array, cgltf_accessor *accessor, float *out) {
  switch (accessor->type) {
  case cgltf_type_vec2: {

    for (int i = 0; i < accessor->count; i++) {
      cgltf_accessor_read_float(accessor, i, &out[i * 2], 2);
    }
    break;
  }
  case cgltf_type_vec3: {

    for (int i = 0; i < accessor->count; i++) {
      cgltf_accessor_read_float(accessor, i, &out[i * 3], 3);
    }

    break;
  }
  case cgltf_type_vec4: {

    for (int i = 0; i < accessor->count; i++) {
      cgltf_accessor_read_float(accessor, i, &out[i * 4], 4);
    }
    break;
  }
  case cgltf_type_mat4: {
    for (int i = 0; i < accessor->count; i++) {
      cgltf_accessor_read_float(accessor, i, &out[i * 16], 16);
    }
    break;
  }
  case cgltf_type_scalar: {

    for (int i = 0; i < accessor->count; i++) {
      float number;
      cgltf_accessor_read_float(accessor, i, &number, 1);
      array_add(array, &number);
    }

    break;
  }
  }

  switch (accessor->component_type) {
  case cgltf_component_type_r_16:
    /* code */
    break;

  default:
    break;
  }
}

void pe_load_attribute(Array* vertex_array, cgltf_attribute *attribute) {
  switch (attribute->type) {
  case cgltf_attribute_type_position: {
    LOG("#### Vertex count: %i\n", (int)attribute->data->count);
    vec3 vertices_position[attribute->data->count];
    ZERO(vertices_position);

    array_init(vertex_array, sizeof(PVertex), attribute->data->count);

    pe_loader_read_accessor(
        vertex_array, attribute->data,
        (float *)vertices_position); // TODO maybe here we broke something

    for (int i = 0; i < attribute->data->count; i++) {
      PVertex vertex;
      ZERO(vertex);
      glm_vec3_copy(vertices_position[i], vertex.position);
      array_add(vertex_array, &vertex);
    }
    break;
  }
  case cgltf_attribute_type_texcoord: {

    vec2 uvs[attribute->data->count];
    ZERO(uvs);

    pe_loader_read_accessor(vertex_array, attribute->data, (float*)uvs);

    for (int i = 0; i < attribute->data->count; i++) {
      PVertex *vertex = array_get(vertex_array, i);
      vertex->uv[0] = uvs[i][0];
      vertex->uv[1] = uvs[i][1];
    }

    break;
  }
  case cgltf_attribute_type_normal: {
    // LOG("Normal attribute \n");
    vec3 normals[attribute->data->count];
    ZERO(normals);

    //pe_loader_read_accessor(attribute->data, normals);TODO normals

    for (int i = 0; i < attribute->data->count; i++) {
      PVertex *vertex = array_get(vertex_array, i);
      glm_vec3_copy(normals[i], vertex->normal);
    }

    break;
  }



  } // end switch

  if (attribute->data->has_min) {

    //glm_vec3_copy(attribute->data->min, selected_model->min);
  }
  if (attribute->data->has_max) {

    //glm_vec3_copy(attribute->data->max, selected_model->max);
  }
}

void pe_loader_mesh_load_primitive(Array *vertex_array, Array *index_array,
                                   cgltf_primitive *primitive) {

  for (int i = 0; i < primitive->attributes_count; i++) {
    pe_load_attribute(vertex_array, &primitive->attributes[i]);
  }

  pe_loader_mesh_read_accessor_indices(index_array, primitive->indices);
}

void pe_load_mesh(PModel *model, cgltf_mesh *mesh) {

  for (int i = 0; i < mesh->primitives_count; i++) {
    pe_loader_mesh_load_primitive(&model->vertex_array, &model->index_array,
                                  &mesh->primitives[i]);
  }
  
}

int pe_node_load(PModel* model, cgltf_node *in_cgltf_node) {


  if (in_cgltf_node->mesh == NULL) {

    // LOG("********* No mesh in node");
  }

  if (in_cgltf_node->mesh != NULL) {
    LOG("Loading GLTF mesh\n");
    // check_LOD_names(in_cgltf_node);
    pe_load_mesh(model, in_cgltf_node->mesh);
  }

  if (in_cgltf_node->skin != NULL) {

  }


  if (in_cgltf_node->children_count == 0 && in_cgltf_node->mesh == NULL) {
    return 1;
  }

  for (int i = 0; i < in_cgltf_node->children_count; i++) {
    pe_node_load(model, in_cgltf_node->children[i]);
  }
}

cgltf_result pe_loader_model_from_memory(PModel* model, void *gltf_data, u32 size,
                                         const char *path) {
  cgltf_options options = {0};
  cgltf_data *data = NULL;

  cgltf_result result = cgltf_parse(&options, gltf_data, size, &data);
  if (result != cgltf_result_success)
    return result;

  current_data = data;

  result = cgltf_load_buffers(&options, data, path);
  if (result != cgltf_result_success)
    return result;

  if (!data || !data->scene) {
    return cgltf_result_invalid_options;
  }


  if (data->skins_count >= 1) {

  }

  //  LOG("******************Loading nodes");

  for (int i = 0; i < data->scene->nodes_count; i++) {
    pe_node_load(model, data->scene->nodes[i]);
  }


  if (data->animations_count >= 1) {

  }

  cgltf_free(data);
  current_data = NULL;

  return result;
}

PModel *pe_vk_load_model(PModel* model, const char *path) {

  pe_load_model_path(model, path);

  model->vertex_buffer = pe_vk_create_buffer(model->vertex_array.bytes_size,
                                             model->vertex_array.data,
                                             VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
  model->index_buffer = pe_vk_create_buffer(model->index_array.bytes_size,
                                            model->index_array.data,
                                            VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

  pe_vk_create_uniform_buffers(model);
  pe_vk_descriptor_pool_create(model);
 
  //init model matrix
  glm_mat4_identity(model->model_mat);
  //setup Uniform Buffer Object
  glm_mat4_copy(model->model_mat,model->uniform_buffer_object.model);
  glm_mat4_copy(main_camera.projection, model->uniform_buffer_object.projection);
  glm_mat4_copy(main_camera.view, model->uniform_buffer_object.view);

  return model;
}

void pe_clean_model(PModel* model){
  for(int i = 0; i < model->uniform_buffers_memory.count; i++){
    VkDeviceMemory* memory = array_get(&model->uniform_buffers_memory, i);
    //printf("Freeying uniform buffermemory %p\n", *memory);
    vkFreeMemory(vk_device, *memory, NULL);
  }

  vkDestroyDescriptorPool(vk_device, model->descriptor_pool, NULL);

  pe_vk_clean_shader(&model->shader);

  vkFreeMemory(vk_device,model->index_buffer.memory, NULL); 
  vkFreeMemory(vk_device,model->vertex_buffer.memory, NULL); 

}

int pe_load_model_path(PModel* model, const char *path) {
  File new_file;

  if (load_file(path, &new_file) == -1) {
    LOG("**** load_file() error\n");
    return -1;
  }

  cgltf_result result =
      pe_loader_model_from_memory(model, new_file.data, new_file.size_in_bytes, path);

  if (result != cgltf_result_success) {
    LOG("Model no loaded: %s \n", new_file.path);
    if (result == cgltf_result_io_error) {
      LOG("Buffer no loaded: %s \n", new_file.path);
      LOG("IO ERROR\n");
    }
    return -1;
  }

  close_file(&new_file);

  // LOG("glTF2 loaded: %s. \n",path);

  return 1;//TODO
}
