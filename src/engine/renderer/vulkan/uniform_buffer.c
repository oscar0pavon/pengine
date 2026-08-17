#include "uniform_buffer.h"
#include "ThirdParty/cglm/affine.h"
#include "ThirdParty/cglm/types.h"
#include "ThirdParty/cglm/vec4.h"
#include "engine/components/skinned_mesh_component.h"
#include "engine/types.h"
#include "vk_buffer.h"
#include <engine/engine.h>
#include <vulkan/vulkan_core.h>

PUniformBufferObject ubo;

void pe_vk_ubo_init() {
  ZERO(ubo);
  glm_mat4_identity(ubo.projection);
  glm_mat4_identity(ubo.view);
  glm_mat4_identity(ubo.model);
}

PBufferCreateInfo pe_vk_create_shader_storage_buffer(size_t size) {

  PBufferCreateInfo info;
  ZERO(info);
  info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  info.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

  info.size = size;

  pe_vk_buffer_create(&info);
  return info;
}
PBufferCreateInfo pe_vk_uniform_buffer_create_buffer(size_t size) {

  PBufferCreateInfo info;
  ZERO(info);
  info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  info.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

  info.size = size;

  pe_vk_buffer_create(&info);
  return info;
}
void pe_vk_create_shader_storage_buffers(PSkinnedMeshComponent *skinned) {

  array_init(&skinned->shader_storage_buffers, sizeof(VkBuffer), 4);
  array_init(&skinned->shader_storage_buffers_memory, sizeof(VkDeviceMemory),
             4);

  for (int i = 0; i < 4; i++) {
    // create buffer
    PBufferCreateInfo info = pe_vk_create_shader_storage_buffer(
        skinned->joints.count * sizeof(mat4));
    array_add(&skinned->shader_storage_buffers, &info.buffer);
    array_add(&skinned->shader_storage_buffers_memory, &info.buffer_memory);
  }
}

void pe_vk_create_uniform_buffers(PModel *model) {
  VkDeviceSize buffer_size = sizeof(PUniformBufferObject);

  array_init(&model->uniform_buffers, sizeof(VkBuffer), 4);
  array_init(&model->uniform_buffers_memory, sizeof(VkDeviceMemory), 4);

  for (int i = 0; i < 4; i++) {
    // create buffer
    PBufferCreateInfo info =
        pe_vk_uniform_buffer_create_buffer(sizeof(PUniformBufferObject));
    array_add(&model->uniform_buffers, &info.buffer);
    array_add(&model->uniform_buffers_memory, &info.buffer_memory);
  }

  // buffer_color = pe_vk_uniform_buffer_create_buffer(sizeof(PEColorShader));
}

void pe_vk_memory_copy(size_t size, VkDeviceMemory *memory, void *in_data) {

  void *data;
  vkMapMemory(vk_device, *(memory), 0, size, 0, &data);
  memcpy(data, in_data, size);
  vkUnmapMemory(vk_device, *(memory));
}

void pe_vk_uniform_buffer_update_skeletal(uint32_t image_index) {

  PUniformBufferObject pawn_ubo;

  ZERO(pawn_ubo);

  glm_mat4_identity(pawn_ubo.model);

  // glm_rotate(pawn_ubo.model, 90.f, VEC3(1, 0, 0));
  // glm_translate(pawn_ubo.model, VEC3(0, 5, 0));
  glm_mat4_copy(main_camera.projection, pawn_ubo.projection);
  glm_mat4_copy(main_camera.view, pawn_ubo.view);

  pawn_ubo.projection[1][1] *= -1;

  PUniformBufferObject buffers[] = {pawn_ubo};

  VkDeviceMemory *memory =
      array_get(&anim_model->uniform_buffers_memory, image_index);

  pe_vk_memory_copy(sizeof(buffers), memory, buffers);

  VkDeviceMemory *skin_memory = array_get(
      &pe_curr_skin_loading->shader_storage_buffers_memory, image_index);

  pe_vk_memory_copy(pe_curr_skin_loading->joints.count * sizeof(mat4),
                    skin_memory,
                    pe_curr_skin_loading->node_uniform.joints_matrix);
}
void pe_vk_uniform_buffer_update_two(uint32_t image_index) {

  PUniformBufferObject pawn_ubo;

  ZERO(pawn_ubo);

  glm_mat4_identity(pawn_ubo.model);

  glm_rotate(pawn_ubo.model, 90.f, VEC3(1, 0, 0));
  glm_translate(pawn_ubo.model, VEC3(0, 5, 0));
  glm_mat4_copy(main_camera.projection, pawn_ubo.projection);
  glm_mat4_copy(main_camera.view, pawn_ubo.view);

  pawn_ubo.projection[1][1] *= -1;

  PUniformBufferObject buffers[] = {pawn_ubo};

  VkDeviceMemory *memory =
      array_get(&test_model2->uniform_buffers_memory, image_index);

  pe_vk_memory_copy(sizeof(buffers), memory, buffers);
}
void pe_vk_uniform_buffer_update_one(uint32_t image_index) {

  PUniformBufferObject pawn_ubo;

  ZERO(pawn_ubo);

  glm_mat4_identity(pawn_ubo.model);

  // glm_rotate(pawn_ubo.model, 90.f, VEC3(1, 0, 0));

  glm_mat4_copy(main_camera.projection, pawn_ubo.projection);
  glm_mat4_copy(main_camera.view, pawn_ubo.view);

  glm_vec4_copy(VEC4(0, 2, 0, 1), pawn_ubo.light_position);

  pawn_ubo.projection[1][1] *= -1;

  PUniformBufferObject buffers[] = {pawn_ubo};

  VkDeviceMemory *memory =
      array_get(&test_model->uniform_buffers_memory, image_index);

  pe_vk_memory_copy(sizeof(buffers), memory, buffers);
}
