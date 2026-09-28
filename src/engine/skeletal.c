#include "skeletal.h"

#include "renderer/vk_buffer.h"
#include "renderer/vulkan.h"
#include <engine/array.h>
#include <engine/macros.h>
#include <string.h>

void pe_skeletal_inverse_kinematic(PSkin *skin) {}

//INFO sized and filled to match skinned.vert's SSBO exactly:
//`buffer JointMatrices { mat4 joint_matrices[]; };` is only the array, no
//leading count, but SkeletalNodeUniform (built for a uniform buffer, which
//can't be a bare unsized array) puts one there for joint_count. copying the
//whole struct across, as pe_vk_create_uniform_buffers()'s equivalent does
//for a real uniform buffer, would shift every matrix the shader reads by
//4 bytes - this only ever sizes and copies node_uniform.joints_matrix itself
void pe_vk_skin_create_storage_buffers(PSkin *skin) {
  u32 count = pe_vk_targets_max_images_count();

  array_init(&skin->shader_storage_buffers, sizeof(VkBuffer), count);
  array_init(&skin->shader_storage_buffers_memory, sizeof(VkDeviceMemory),
            count);

  for (u32 i = 0; i < count; i++) {
    PBufferCreateInfo info;
    ZERO(info);
    info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    info.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    info.size = sizeof(skin->node_uniform.joints_matrix);

    pe_vk_create_buffer_memory(&info);

    array_add(&skin->shader_storage_buffers, &info.buffer);
    array_add(&skin->shader_storage_buffers_memory, &info.buffer_memory);
  }
}

//call after pe_anim_nodes_update() has filled skin->node_uniform for this
//frame, once per swap chain image index the frame draws into
void pe_vk_skin_send_storage_buffer(PSkin *skin, u32 image_index) {
  VkDeviceMemory *memory =
      array_get(&skin->shader_storage_buffers_memory, image_index);

  void *data;
  vkMapMemory(vk_device, *memory, 0, sizeof(skin->node_uniform.joints_matrix),
             0, &data);
  memcpy(data, &skin->node_uniform.joints_matrix,
        sizeof(skin->node_uniform.joints_matrix));
  vkUnmapMemory(vk_device, *memory);
}

void get_global_matrix(Node *node, mat4 out_mat) {
  mat4 local;
  glm_mat4_identity(local);
  get_local_matrix(node, local);
  Node *node_parent = node->parent;
  while (node_parent != NULL) {
    mat4 local_mat_parent;
    get_local_matrix(node_parent, local_mat_parent);
    glm_mul(local_mat_parent, local, local);
    node_parent = node_parent->parent;
  }
  glm_mat4_copy(local, out_mat);
}
void get_local_matrix(Node *node, mat4 out_mat) {
  mat4 translation;
  glm_mat4_identity(translation);
  glm_translate(translation, node->translation);
  mat4 rot;
  glm_mat4_identity(rot);
  glm_quat_mat4(node->rotation, rot);
  glm_mul(translation, rot, out_mat);
}
