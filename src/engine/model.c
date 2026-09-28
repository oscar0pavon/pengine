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

#include "skeletal.h"
#include "animation/animation.h"

cgltf_data *current_data;

//INFO set by pe_vk_load_skin() before the parse below, the same way
//current_data is: pe_node_load() and the skin/animation blocks in
//pe_loader_model_from_memory() only have this one call stack to fill a
//PSkin in, since the cgltf_skin and cgltf_animation they read from are
//freed at the end of it
PSkin *current_skin;


//INFO only the first call sizes the array; the calls after it, one per
//further primitive of the same mesh, add their indices to that same one.
//array_init() cannot be left to refuse the later calls itself - it zeroes
//the Array before testing whether it was already initialized, so its own
//guard never fires and it would start the array over every primitive,
//leaving a mesh with only its last primitive's indices
void pe_loader_mesh_read_accessor_indices(Array *index_array,
                                          cgltf_accessor *accessor) {
  if (index_array->initialized == false) {
    switch (accessor->component_type) {
    case cgltf_component_type_r_8u:
      array_init(index_array, sizeof(u8), accessor->count);
      break;
    case cgltf_component_type_r_16u:
      array_init(index_array, sizeof(unsigned short), accessor->count);
      break;
    case cgltf_component_type_r_32u:
      array_init(index_array, sizeof(unsigned int), accessor->count);
      break;
    default:
      break;
    }
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

    pe_loader_read_accessor(vertex_array, attribute->data, (float *)normals);

    for (int i = 0; i < attribute->data->count; i++) {
      PVertex *vertex = array_get(vertex_array, i);
      glm_vec3_copy(normals[i], vertex->normal);
    }

    break;
  }
  //INFO JOINTS_0 is usually unsigned byte or short, never normalized, so
  //cgltf_accessor_read_float() (via cgltf_component_read_float()) hands back
  //the plain index 0..255 as a float rather than scaling it - exactly what
  //int(joint.x) in the skinned vertex shader wants
  case cgltf_attribute_type_joints: {
    vec4 joints[attribute->data->count];
    ZERO(joints);

    pe_loader_read_accessor(vertex_array, attribute->data, (float *)joints);

    for (int i = 0; i < attribute->data->count; i++) {
      PVertex *vertex = array_get(vertex_array, i);
      glm_vec4_copy(joints[i], vertex->joint);
    }

    break;
  }
  case cgltf_attribute_type_weights: {
    vec4 weights[attribute->data->count];
    ZERO(weights);

    pe_loader_read_accessor(vertex_array, attribute->data, (float *)weights);

    for (int i = 0; i < attribute->data->count; i++) {
      PVertex *vertex = array_get(vertex_array, i);
      glm_vec4_copy(weights[i], vertex->weight);
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

//the accessor decides whether an index is one, two or four bytes wide
static u32 pe_loader_index_at(Array *index_array, u32 position) {
  u32 index = 0;
  memcpy(&index, array_get(index_array, position),
         index_array->element_bytes_size);
  return index;
}

//INFO NORMAL is optional in gltf, and a file without it asks the client to
//calculate flat normals. cube.glb - the chess board, and every square copied
//from it - carries only POSITION and TEXCOORD_0, so without this the normals
//stay zero, normalize() in the fragment shader answers NaN and the board draws
//solid black
static void pe_loader_flat_normals(Array *vertex_array, Array *index_array) {

  if (vertex_array->count == 0 || index_array->count < 3)
    return;

  for (u32 i = 0; i + 2 < index_array->count; i += 3) {

    u32 index[3];
    for (int corner = 0; corner < 3; corner++)
      index[corner] = pe_loader_index_at(index_array, i + corner);

    if (index[0] >= vertex_array->count || index[1] >= vertex_array->count ||
        index[2] >= vertex_array->count)
      continue;

    PVertex *corner[3];
    for (int c = 0; c < 3; c++)
      corner[c] = array_get(vertex_array, index[c]);

    vec3 edge1, edge2, normal;
    glm_vec3_sub(corner[1]->position, corner[0]->position, edge1);
    glm_vec3_sub(corner[2]->position, corner[0]->position, edge2);
    glm_vec3_cross(edge1, edge2, normal);

    if (glm_vec3_norm(normal) == 0)
      continue;

    glm_vec3_normalize(normal);

    for (int c = 0; c < 3; c++)
      glm_vec3_copy(normal, corner[c]->normal);
  }
}

//INFO a mesh's attributes are read once, from its first primitive, and every
//primitive after that only adds its own indices to the same shared vertex
//array. that is the shape a mesh split by material has - one vertex set,
//one primitive per material, each naming the same POSITION/NORMAL/...
//accessors and differing only in which slice of indices it draws - and it
//is what m22gltf writes for a model whose 70 batches all index the same
//2737 vertices. a mesh whose primitives carried genuinely different vertex
//data would need each one appended with its own vertex range and its
//indices offset past the ranges before it, which this does not do.
//
//reading every primitive's attributes instead, as this used to, rebuilt the
//vertex array per primitive and left only the last primitive's indices,
//because array_init() zeroes an Array before it checks whether it was
//already initialized - so the guard meant to refuse a second init never
//fires and each call silently starts the array over
void pe_load_mesh(PModel *model, cgltf_mesh *mesh) {

  if (mesh->primitives_count == 0)
    return;

  bool has_normals = false;
  for (int i = 0; i < mesh->primitives[0].attributes_count; i++) {
    if (mesh->primitives[0].attributes[i].type == cgltf_attribute_type_normal)
      has_normals = true;
    pe_load_attribute(&model->vertex_array, &mesh->primitives[0].attributes[i]);
  }

  for (int i = 0; i < mesh->primitives_count; i++)
    pe_loader_mesh_read_accessor_indices(&model->index_array,
                                         mesh->primitives[i].indices);

  if (!has_normals)
    pe_loader_flat_normals(&model->vertex_array, &model->index_array);
}

//INFO joints are only ever bound skin->joints and pe_load_skin()'s own
//cgltf_joints, never freed early, so this is safe to call from
//pe_load_animations() right after
static Node *pe_find_joint_node(PSkin *skin, cgltf_skin *in_skin,
                                cgltf_node *target) {
  for (u32 i = 0; i < in_skin->joints_count; i++)
    if (in_skin->joints[i] == target)
      return array_get(&skin->joints, i);
  return NULL;
}

//one PSkin per file is all this reads, which is all m22gltf (and every other
//glTF exporter meant for a single rigged character) ever writes
static void pe_load_skin(PSkin *skin, cgltf_skin *in_skin) {
  u32 joint_count = in_skin->joints_count;
  cgltf_node *cgltf_joints[joint_count];

  array_init(&skin->joints, sizeof(Node), joint_count);
  array_init(&skin->inverse_bind_matrices, sizeof(mat4), joint_count);

  for (u32 i = 0; i < joint_count; i++) {
    cgltf_node *in_node = in_skin->joints[i];
    cgltf_joints[i] = in_node;

    Node node;
    ZERO(node);
    node.id = i;
    snprintf(node.name, sizeof(node.name), "%s",
            in_node->name ? in_node->name : "");

    if (in_node->has_translation)
      glm_vec3_copy(in_node->translation, node.translation);
    if (in_node->has_rotation)
      glm_vec4_copy(in_node->rotation, node.rotation);
    else
      glm_quat_identity(node.rotation);

    array_add(&skin->joints, &node);

    mat4 inverse_bind;
    glm_mat4_identity(inverse_bind);
    if (in_skin->inverse_bind_matrices != NULL)
      cgltf_accessor_read_float(in_skin->inverse_bind_matrices, i,
                                (float *)inverse_bind, 16);
    array_add(&skin->inverse_bind_matrices, inverse_bind);
  }

  //INFO parents are only wired up now that every joint has its final,
  //settled place in skin->joints - array_add() above may have moved the
  //array when it grew, and a parent pointer taken before that would dangle
  for (u32 i = 0; i < joint_count; i++) {
    Node *child = array_get(&skin->joints, i);
    child->parent = NULL;

    cgltf_node *parent = cgltf_joints[i]->parent;
    for (u32 p = 0; p < joint_count; p++) {
      if (cgltf_joints[p] == parent) {
        child->parent = array_get(&skin->joints, p);
        break;
      }
    }
  }

  skin->node_uniform.joint_count = joint_count;
}

//translation and rotation channels only: PSkin's Node has no scale, matching
//what m22gltf's bones actually need it for (Blizzard's own tooling, not
//gear or squash-and-stretch effects)
static void pe_load_animations(PSkin *skin, cgltf_data *data) {
  if (data->skins_count == 0)
    return;
  cgltf_skin *in_skin = &data->skins[0];

  array_init(&skin->animations, sizeof(Animation), data->animations_count);

  for (u32 a = 0; a < data->animations_count; a++) {
    cgltf_animation *in_animation = &data->animations[a];

    Animation animation;
    ZERO(animation);
    snprintf(animation.name, sizeof(animation.name), "%s",
            in_animation->name ? in_animation->name : "");

    array_init(&animation.channels, sizeof(AnimationChannel),
              in_animation->channels_count);

    for (u32 c = 0; c < in_animation->channels_count; c++) {
      cgltf_animation_channel *in_channel = &in_animation->channels[c];

      unsigned short path_type;
      int floats_per;
      if (in_channel->target_path == cgltf_animation_path_type_translation) {
        path_type = PATH_TYPE_TRANSLATION;
        floats_per = 3;
      } else if (in_channel->target_path ==
                cgltf_animation_path_type_rotation) {
        path_type = PATH_TYPE_ROTATION;
        floats_per = 4;
      } else {
        continue;
      }

      Node *node = pe_find_joint_node(skin, in_skin, in_channel->target_node);
      if (node == NULL)
        continue;

      cgltf_animation_sampler *in_sampler = in_channel->sampler;
      u32 key_count = in_sampler->input->count;

      AnimationChannel channel;
      ZERO(channel);
      channel.path_type = path_type;
      channel.node = node;

      array_init(&channel.sampler.inputs, sizeof(float), key_count);
      array_init(&channel.sampler.outputs, sizeof(float) * floats_per,
                key_count);

      for (u32 k = 0; k < key_count; k++) {
        float time;
        cgltf_accessor_read_float(in_sampler->input, k, &time, 1);
        array_add(&channel.sampler.inputs, &time);

        float value[4];
        cgltf_accessor_read_float(in_sampler->output, k, value, floats_per);
        array_add(&channel.sampler.outputs, value);
      }

      if (key_count > 0) {
        float last_time = *(float *)array_get_last(&channel.sampler.inputs);
        if (last_time > animation.end)
          animation.end = last_time;
      }

      array_add(&animation.channels, &channel);
    }

    array_add(&skin->animations, &animation);
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


  if (current_skin != NULL && data->skins_count >= 1)
    pe_load_skin(current_skin, &data->skins[0]);

  //  LOG("******************Loading nodes");

  for (int i = 0; i < data->scene->nodes_count; i++) {
    pe_node_load(model, data->scene->nodes[i]);
  }


  if (current_skin != NULL && data->animations_count >= 1)
    pe_load_animations(current_skin, data);

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

  pe_vk_create_uniform_buffers(model, &main_render_target);
  pe_vk_descriptor_pool_create(model, &main_render_target);

  //INFO a model is not drawable until its descriptor sets exist and point at
  //its uniform buffers - pe_vk_draw_model() binds set[image_index] every draw.
  //the pool alone was not enough and left the sets array empty
  pe_vk_create_descriptor_sets(model, pe_vk_descriptor_set_layout,
                               &main_render_target);
  pe_vk_descriptor_update(model, &main_render_target);

  //init model matrix
  glm_mat4_identity(model->model_mat);
  //setup Uniform Buffer Object
  glm_mat4_copy(model->model_mat,model->uniform_buffer_object.model);
  glm_mat4_copy(main_camera.projection, model->uniform_buffer_object.projection);
  glm_mat4_copy(main_camera.view, model->uniform_buffer_object.view);

  return model;
}

//INFO reuses pe_vk_load_model() as-is for the mesh, vertex/index buffers and
//uniform buffer - current_skin just gives pe_loader_model_from_memory()
//somewhere to put the skin and animations it finds along the way. what this
//does not yet do: model's descriptor sets and pool here are still the plain
//(uniform buffer only) ones pe_vk_load_model() always makes, not the
//uniform+texture+joint matrices SSBO layout skinned.vert needs - drawing a
//skin still wants a pipeline, descriptor layout and per frame joint matrix
//upload of its own, none of which exist yet
PModel *pe_vk_load_skin(PSkin *skin, PModel *model, const char *path) {
  ZERO(*skin);
  skin->mesh = model;

  current_skin = skin;
  pe_vk_load_model(model, path);
  current_skin = NULL;

  return model;
}

//INFO one more thing to draw with geometry that is already on the gpu. the
//vertex and index buffers are shared with source, but the copy gets its own
//uniform buffers and descriptor sets, because those carry the per instance
//model matrix. sharing them would make every copy sit where the last one moved
PModel *pe_vk_model_instance(PModel *model, PModel *source) {

  memcpy(model, source, sizeof(PModel));

  ZERO(model->uniform_buffers);
  ZERO(model->uniform_buffers_memory);
  ZERO(model->descriptor_sets);

  pe_vk_create_uniform_buffers(model, &main_render_target);
  pe_vk_descriptor_pool_create(model, &main_render_target);
  pe_vk_create_descriptor_sets(model, pe_vk_descriptor_set_layout,
                               &main_render_target);
  pe_vk_descriptor_update(model, &main_render_target);

  glm_mat4_identity(model->model_mat);
  glm_mat4_copy(model->model_mat, model->uniform_buffer_object.model);

  return model;
}

void pe_model_transform_reset(PModel *model) {
  glm_mat4_identity(model->model_mat);
  glm_vec3_zero(model->position);
}

void pe_model_transform(PModel *model, vec3 position, float angle, vec3 axis,
                        vec3 scale) {
  glm_mat4_identity(model->model_mat);
  glm_translate(model->model_mat, position);
  glm_rotate(model->model_mat, glm_rad(angle), axis);
  glm_scale(model->model_mat, scale);

  glm_vec3_copy(position, model->position);
}

//INFO the translation lives in the fourth column, so it can be replaced
//without touching the rotation and scale in the upper 3x3 - composing a fresh
//translate onto the matrix instead would move it by position every call
void pe_model_set_position(PModel *model, vec3 position) {
  glm_vec3_copy(position, model->model_mat[3]);
  glm_vec3_copy(position, model->position);
}

void pe_model_translate(PModel *model, vec3 offset) {
  glm_translate(model->model_mat, offset);
  glm_vec3_copy(model->model_mat[3], model->position);
}

void pe_model_rotate(PModel *model, float angle, vec3 axis) {
  glm_rotate(model->model_mat, glm_rad(angle), axis);
}

void pe_model_scale(PModel *model, vec3 scale) {
  glm_scale(model->model_mat, scale);
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
