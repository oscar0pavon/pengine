#include "model.h"
#include "engine/array.h"
#include "engine/components/skinned_mesh_component.h"

#define CGLTF_IMPLEMENTATION
#include "../ThirdParty/cglm/vec3.h"
#include "../ThirdParty/cgltf.h"
#include "stdio.h"

#include "file_loader.h"

#include "engine.h"

#include "vertex.h"

#include <engine/renderer/vulkan/vk_vertex.h>

cgltf_data *current_data;
cgltf_animation *current_animation;
cgltf_animation_channel *current_channel;
cgltf_animation_sampler *current_sampler;
bool copy_nodes = false;
int nodes_counter = 0;

Array *actual_vertex_array;
Array *actual_index_array;
Array *current_array;

int models_parsed = 0;

void pe_loader_mesh_read_accessor_indices(cgltf_accessor *accessor) {
  switch (accessor->component_type) {
  case cgltf_component_type_r_8:
    break;
  case cgltf_component_type_r_16:
    break;
  case cgltf_component_type_r_8u:

    array_init(actual_index_array, sizeof(u8), accessor->count);
    break;
  case cgltf_component_type_r_16u:

    array_init(actual_index_array, sizeof(unsigned short), accessor->count);
    break;
  case cgltf_component_type_r_32f:
    break;
  case cgltf_component_type_r_32u:
    array_init(actual_index_array, sizeof(unsigned int), accessor->count);
    break;
  }
  for (size_t i = 0; i < accessor->count; i++) {
    size_t index = cgltf_accessor_read_index(accessor, i);
    array_add(actual_index_array, &index);
  }
}

/*Read accessor and allocate data in current_array or actual_vertex_array */
void pe_loader_read_accessor(cgltf_accessor *accessor, float *out) {
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

    LOG("### Scalar");
    for (int i = 0; i < accessor->count; i++) {
      float number;
      cgltf_accessor_read_float(accessor, i, &number, 1);
      array_add(current_array, &number);
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
void pe_debug_accesor_type(char *message, cgltf_accessor *accessor) {

  switch (accessor->type) {
  case cgltf_type_vec2: {

    LOG("######## %s data is VEC2\n", message);
    break;
  }
  case cgltf_type_vec3: {
    LOG("######## %s data is VEC3\n", message);
    break;
  }
  case cgltf_type_vec4: {

    LOG("######## %s data is VEC4\n", message);
    break;
  }
  case cgltf_type_scalar: {

    LOG("######## %s data is scalar\n", message);
    break;
  }

  case cgltf_type_mat4: {

    LOG("######## %s data is MAT4\n", message);
    break;
  }
  default:
    break;
  }
}

void pe_loader_attribute(cgltf_attribute *attribute) {
  switch (attribute->type) {
  case cgltf_attribute_type_position: {
    LOG("#### Vertex count: %i\n", (int)attribute->data->count);
    vec3 vertices_position[attribute->data->count];
    ZERO(vertices_position);

    if (actual_vertex_array == NULL) {
      LOG("Actual vertex array is NULL\n");
    }

    array_init(actual_vertex_array, sizeof(PVertex), attribute->data->count);

    pe_loader_read_accessor(attribute->data, vertices_position);

    for (int i = 0; i < attribute->data->count; i++) {
      PVertex vertex;
      ZERO(vertex);
      glm_vec3_copy(vertices_position[i], vertex.position);
      array_add(actual_vertex_array, &vertex);
    }
    break;
  }
  case cgltf_attribute_type_texcoord: {

    vec2 uvs[attribute->data->count];
    ZERO(uvs);

    pe_loader_read_accessor(attribute->data, uvs);

    for (int i = 0; i < attribute->data->count; i++) {
      PVertex *vertex = array_get(actual_vertex_array, i);
      vertex->uv[0] = uvs[i][0];
      vertex->uv[1] = uvs[i][1];
    }

    break;
  }
  case cgltf_attribute_type_normal: {
    // LOG("Normal attribute \n");
    vec3 normals[attribute->data->count];
    ZERO(normals);

    pe_loader_read_accessor(attribute->data, normals);

    for (int i = 0; i < attribute->data->count; i++) {
      PVertex *vertex = array_get(actual_vertex_array, i);
      glm_vec3_copy(normals[i], vertex->normal);
    }

    break;
  }

  case cgltf_attribute_type_joints: {
    vec4 joints[attribute->data->count];
    ZERO(joints);
    // pe_debug_accesor_type("Joints", attribute->data);
    pe_loader_read_accessor(attribute->data, joints);
    for (int i = 0; i < attribute->data->count; i++) {
      PVertex *vertex = array_get(actual_vertex_array, i);
      glm_vec4_copy(joints[i], vertex->joint);
      // LOG("#### Vertex Joint attribute %f, %f , %f ,%f\n", vertex->joint[0],
      //    vertex->joint[1], vertex->joint[2], vertex->joint[3]);
    }
    // LOG("##### Joint load\n");

    break;
  }

  case cgltf_attribute_type_weights: {
    vec4 weights[attribute->data->count];
    ZERO(weights);

    // pe_debug_accesor_type("Weight", attribute->data);

    pe_loader_read_accessor(attribute->data, weights);

    for (int i = 0; i < attribute->data->count; i++) {
      PVertex *vertex = array_get(actual_vertex_array, i);
      glm_vec4_copy(weights[i], vertex->weight);
      // LOG("## Weight in vertex\n");
      // LOG("Vertex weight %f %f %f %f\n", vertex->weight[0],
      // vertex->weight[1],
      //    vertex->weight[2], vertex->weight[3]);

      // LOG("## Weights array\n");
      // LOG("######array %f %f %f %f\n", weights[i][0], weights[i][1],
      //     weights[i][2], weights[i][3]);
    }

    // LOG("##### Weighs load\n");
    break;
  }

  } // end switch

  if (attribute->data->has_min) {

    glm_vec3_copy(attribute->data->min, selected_model->min);
  }
  if (attribute->data->has_max) {

    glm_vec3_copy(attribute->data->max, selected_model->max);
  }
}

void pe_loader_mesh_load_primitive(cgltf_primitive *primitive) {

  for (int i = 0; i < primitive->attributes_count; i++) {
    pe_loader_attribute(&primitive->attributes[i]);
  }

  pe_loader_mesh_read_accessor_indices(primitive->indices);
}

void pe_loader_mesh(cgltf_mesh *mesh) {

  for (int i = 0; i < mesh->primitives_count; i++) {
    new_empty_model();
    actual_vertex_array = &selected_model->vertex_array;
    actual_index_array = &selected_model->index_array;
    pe_loader_mesh_load_primitive(&mesh->primitives[i]);
    // pe_th_exec_in(pe_th_render_id, &GPU_buffers_create_for_model,
    //               selected_model);
    GPU_buffers_create_for_model(selected_model);
    while (!selected_model->gpu_ready) {
    };
    models_parsed++;
  }
}

void check_LOD_names(cgltf_node *node) {
  int node_name_size = strlen(node->name);
  char name[node_name_size];
  strcpy(name, node->name);

  for (int n = 0; n < node_name_size; n++) {
    if (name[n] == '_') {
      if (strcmp("LOD0", &name[n] + 1) == 0) {
        LOG("Found LOD0\n");
        break;
      }
      if (strcmp("LOD1", &name[n] + 1) == 0) {
        LOG("Found LOD1\n");
        break;
      }

      if (strcmp("LOD2", &name[n] + 1) == 0) {
        LOG("Found LOD2\n");
        break;
      }
    }
  }
}

void load_current_sampler_to_channel(AnimationChannel *channel) {
  AnimationSampler sampler;
  memset(&sampler, 0, sizeof(AnimationSampler));
  array_init(&sampler.inputs, sizeof(float), current_sampler->input->count);

  current_array = &sampler.inputs;

  float inputs[current_sampler->input->count];
  pe_loader_read_accessor(current_sampler->input, inputs);

  if (channel->path_type == PATH_TYPE_ROTATION) {
    array_init(&sampler.outputs, sizeof(float) * 4,
               current_sampler->output->count);
    vec4 outputs[current_sampler->output->count];
    pe_loader_read_accessor(current_sampler->output, outputs);
    memcpy(sampler.outputs.data, outputs, sizeof(outputs));
  } else if (channel->path_type == PATH_TYPE_TRANSLATION) {
    vec3 outputs[current_sampler->output->count];
    array_init(&sampler.outputs, sizeof(float) * 3,
               current_sampler->output->count);
    pe_loader_read_accessor(current_sampler->output, outputs);
    memcpy(sampler.outputs.data, outputs, sizeof(outputs));
  }

  sampler.outputs.count = current_sampler->output->count;
  current_array = NULL;
  memcpy(&channel->sampler, &sampler, sizeof(AnimationSampler));
}

void load_current_channel_to_animation(Animation *animation) {
  AnimationChannel channel;
  memset(&channel, 0, sizeof(AnimationChannel));
  channel.node = pe_node_by_name(&pe_curr_skin_loading->joints,
                                 current_channel->target_node->name);
  switch (current_channel->target_path) {
  case cgltf_animation_path_type_rotation:
    channel.path_type = PATH_TYPE_ROTATION;
    break;
  case cgltf_animation_path_type_translation:
    channel.path_type = PATH_TYPE_TRANSLATION;
    break;

  default:
    break;
  }

  current_sampler = current_channel->sampler;
  load_current_sampler_to_channel(&channel);

  array_add(&animation->channels, &channel);
}

void load_current_animation() {
  Animation new_animation;
  memset(&new_animation, 0, sizeof(Animation));
  strcpy(new_animation.name, current_animation->name);
  LOG("Loading animation: %s\n", current_animation->name);
  array_init(&new_animation.channels, sizeof(AnimationChannel),
             current_animation->channels_count);
  for (int i = 0; i < current_animation->channels_count; i++) {
    current_channel = &current_animation->channels[i];
    load_current_channel_to_animation(&new_animation);
  }

  float max = 0;
  for (int i = 0; i < new_animation.channels.count; i++) {
    AnimationChannel *channel = array_get(&new_animation.channels, i);
    float *max_from_channel =
        array_get(&channel->sampler.inputs, channel->sampler.inputs.count - 1);
    if (*max_from_channel > max) {
      max = *max_from_channel;
    }
  }
  new_animation.end = max;

  array_add(&pe_curr_skin_loading->animations, &new_animation);
}

int pe_node_load(Node *parent, cgltf_node *in_cgltf_node) {

  if (copy_nodes) {
    if (nodes_counter > 1) {

      Node new_node;
      ZERO(new_node);

      if (in_cgltf_node->parent && parent != NULL) {
        new_node.parent = parent;
      }

      strcpy(new_node.name, in_cgltf_node->name);

      glm_vec3_copy(in_cgltf_node->translation, new_node.translation);
      // glm_vec4_copy(new_node.rotation, in_cgltf_node->rotation);

      array_add(&pe_curr_skin_loading->joints, &new_node);
    }
    nodes_counter++;
  }

  if (in_cgltf_node->mesh == NULL) {

    // LOG("********* No mesh in node");
  }

  if (in_cgltf_node->mesh != NULL) {
    // LOG("Loading GLTF mesh\n");
    // check_LOD_names(in_cgltf_node);
    pe_loader_mesh(in_cgltf_node->mesh);
  }

  if (in_cgltf_node->skin != NULL) {
    current_loaded_component_type = COMPONENT_SKINNED_MESH;
    // pe_debug_accesor_type("Inverse bind matrix",
    //                       in_cgltf_node->skin->inverse_bind_matrices);

    // LOG("Inverse bind matrix count %i\n",
    //     in_cgltf_node->skin->inverse_bind_matrices->count);

    int inverse_bind_matrices_count =
        in_cgltf_node->skin->inverse_bind_matrices->count;

    array_init(&pe_curr_skin_loading->inverse_bind_matrices, sizeof(mat4),
               inverse_bind_matrices_count);

    array_resize(&pe_curr_skin_loading->inverse_bind_matrices,
                 inverse_bind_matrices_count);

    pe_loader_read_accessor(in_cgltf_node->skin->inverse_bind_matrices,
                            pe_curr_skin_loading->inverse_bind_matrices.data);
  }

  Node *loaded_parent = NULL;

  if (pe_curr_skin_loading) {
    loaded_parent = array_pop(&pe_curr_skin_loading->joints);
  }

  if (in_cgltf_node->children_count == 0 && in_cgltf_node->mesh == NULL) {
    return 1;
  }

  for (int i = 0; i < in_cgltf_node->children_count; i++) {
    pe_node_load(loaded_parent, in_cgltf_node->children[i]);
  }
}

cgltf_result pe_loader_model_from_memory(void *gltf_data, u32 size,
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

  current_loaded_component_type = STATIC_MESH_COMPONENT;

  if (data->skins_count >= 1) {
    PSkinnedMeshComponent skin;
    ZERO(skin);
    LOG("Creating skin mesh joints\n");

    array_init(&skin.joints, sizeof(Node), data->nodes_count);
    // ZERO(skin.joints.data);

    LOG("nodes count: %zu \n", data->nodes_count);

    array_add(&pe_arr_skin_loaded, &skin);
    pe_curr_skin_loading = array_pop(&pe_arr_skin_loaded);

    copy_nodes = true;
    current_loaded_component_type = COMPONENT_SKINNED_MESH;
  }

  //  LOG("******************Loading nodes");

  for (int i = 0; i < data->scene->nodes_count; i++) {
    pe_node_load(NULL, data->scene->nodes[i]);
  }

  /* NULL vertex/index array because not needed anymore */
  actual_vertex_array = NULL;
  actual_index_array = NULL;

  if (data->animations_count >= 1) {
    LOG("Loding animation\n");
    array_init(&pe_curr_skin_loading->animations, sizeof(Animation),
               data->animations_count);
    for (int i = 0; i < data->animations_count; i++) {
      current_animation = &data->animations[i];
      load_current_animation();
    }
  }

  cgltf_free(data);
  current_data = NULL;

  return result;
}

PModel *pe_vk_model_load(char *path) {

  actual_model_array = &array_models_loaded;
  pe_loader_model(path);
  PModel *model = selected_model;
#ifdef LINUX
  model->vertex_buffer =
      pe_vk_vertex_create_buffer(&selected_model->vertex_array);
  model->index_buffer =
      pe_vk_vertex_create_index_buffer(&selected_model->index_array);
#endif
  return model;
}

/*
 Load model from gltf and can used by "selected_model"
 pointer, return count of models loaded
*/

int pe_loader_model(const char *path) {
  File new_file;

  if (load_file(path, &new_file) == -1) {
    LOG("**** load_file() error\n");
    return -1;
  }

  cgltf_result result =
      pe_loader_model_from_memory(new_file.data, new_file.size_in_bytes, path);

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

  int model_result = models_parsed;
  models_parsed = 0;
  nodes_counter = 0;
  return model_result;
}
