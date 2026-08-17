#include "descriptor_set.h"

#include "engine/components/skinned_mesh_component.h"
#include "vk_images.h"
#include <engine/array.h>
#include <engine/engine.h>
#include <vulkan/vulkan_core.h>

void pe_vk_descriptor_pool_create(PModel *model) {
  VkDescriptorPoolSize pool_size[3];
  ZERO(pool_size);
  pool_size[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  pool_size[0].descriptorCount = 4;
  pool_size[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  pool_size[1].descriptorCount = 4;
  pool_size[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  pool_size[2].descriptorCount = 4;

  VkDescriptorPoolCreateInfo info;
  ZERO(info);
  info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  info.poolSizeCount = 3;
  info.pPoolSizes = pool_size;
  info.maxSets = 12;

  VKVALID(
      vkCreateDescriptorPool(vk_device, &info, NULL, &model->descriptor_pool),
      "Can't create descriptor pool");
}

void pe_vk_create_descriptor_set_layout_skinned() {
  VkDescriptorSetLayoutBinding uniform;
  ZERO(uniform);
  uniform.binding = 0;
  uniform.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uniform.descriptorCount = 1;
  uniform.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  VkDescriptorSetLayoutBinding texture;
  ZERO(texture);
  texture.binding = 1;
  texture.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  texture.descriptorCount = 1;
  texture.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutBinding skinned;
  ZERO(skinned);
  skinned.binding = 2;
  skinned.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  skinned.descriptorCount = 1;
  skinned.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  VkDescriptorSetLayoutBinding all_binding[] = {uniform, texture, skinned};

  VkDescriptorSetLayoutCreateInfo info;
  ZERO(info);
  info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  info.bindingCount = 3;
  info.pBindings = all_binding;

  VKVALID(vkCreateDescriptorSetLayout(vk_device, &info, NULL,
                                      &pe_vk_descriptor_set_layout_skinned),
          "Can't create Descriptor Set Layout");
}
void pe_vk_create_descriptor_set_layout_with_texture() {
  VkDescriptorSetLayoutBinding uniform;
  ZERO(uniform);
  uniform.binding = 0;
  uniform.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uniform.descriptorCount = 1;
  uniform.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  VkDescriptorSetLayoutBinding texture;
  ZERO(texture);
  texture.binding = 1;
  texture.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  texture.descriptorCount = 1;
  texture.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutBinding all_binding[] = {uniform, texture};

  VkDescriptorSetLayoutCreateInfo info;
  ZERO(info);
  info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  info.bindingCount = 2;
  info.pBindings = all_binding;

  VKVALID(
      vkCreateDescriptorSetLayout(vk_device, &info, NULL,
                                  &pe_vk_descriptor_set_layout_with_texture),
      "Can't create Descriptor Set Layout");
}
void pe_vk_create_descriptor_set_layout() {
  VkDescriptorSetLayoutBinding uniform;
  ZERO(uniform);
  uniform.binding = 0;
  uniform.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uniform.descriptorCount = 1;
  uniform.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  VkDescriptorSetLayoutBinding all_binding[] = {uniform};

  VkDescriptorSetLayoutCreateInfo info;
  ZERO(info);
  info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  info.bindingCount = 1;
  info.pBindings = all_binding;

  VKVALID(vkCreateDescriptorSetLayout(vk_device, &info, NULL,
                                      &pe_vk_descriptor_set_layout),
          "Can't create Descriptor Set Layout");
}
void pe_vk_descriptor_update_skinned(PModel *model,
                                     PSkinnedMeshComponent *skinned) {

  for (int i = 0; i < 4; i++) {

    VkDescriptorBufferInfo info;
    ZERO(info);
    VkBuffer *buffer = array_get(&model->uniform_buffers, i);
    info.buffer = *(buffer);
    info.offset = 0;
    info.range = sizeof(PUniformBufferObject);

    VkDescriptorBufferInfo storage_info;
    ZERO(storage_info);
    VkBuffer *buffer_skinned = array_get(&skinned->shader_storage_buffers, i);
    storage_info.buffer = *(buffer_skinned);
    storage_info.offset = 0;
    storage_info.range = skinned->joints.count * sizeof(mat4);

    VkDescriptorImageInfo image_info;
    ZERO(image_info);
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_info.imageView = pe_vk_texture_image_view;
    image_info.sampler = pe_vk_texture_sampler;

    VkDescriptorSet *set = array_get(&model->descriptor_sets, i);

    VkWriteDescriptorSet des_write[3];
    ZERO(des_write);
    des_write[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    des_write[0].dstSet = *(set);
    des_write[0].dstBinding = 0;
    des_write[0].dstArrayElement = 0;
    des_write[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    des_write[0].descriptorCount = 1;
    des_write[0].pBufferInfo = &info;

    des_write[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    des_write[1].dstSet = *(set);
    des_write[1].dstBinding = 1;
    des_write[1].dstArrayElement = 0;
    des_write[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    des_write[1].descriptorCount = 1;
    des_write[1].pImageInfo = &image_info;

    des_write[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    des_write[2].dstSet = *(set);
    des_write[2].dstBinding = 2;
    des_write[2].dstArrayElement = 0;
    des_write[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    des_write[2].descriptorCount = 1;
    des_write[2].pBufferInfo = &storage_info;

    vkUpdateDescriptorSets(vk_device, 3, des_write, 0, NULL);
  }
}
void pe_vk_descriptor_update(PModel *model) {

  for (int i = 0; i < 4; i++) {

    VkDescriptorBufferInfo info;
    ZERO(info);
    VkBuffer *buffer = array_get(&model->uniform_buffers, i);
    info.buffer = *(buffer);
    info.offset = 0;
    info.range = sizeof(PUniformBufferObject);

    VkDescriptorImageInfo image_info;
    ZERO(image_info);
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_info.imageView = pe_vk_texture_image_view;
    image_info.sampler = pe_vk_texture_sampler;

    VkDescriptorSet *set = array_get(&model->descriptor_sets, i);

    VkWriteDescriptorSet des_write[2];
    ZERO(des_write);
    des_write[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    des_write[0].dstSet = *(set);
    des_write[0].dstBinding = 0;
    des_write[0].dstArrayElement = 0;
    des_write[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    des_write[0].descriptorCount = 1;
    des_write[0].pBufferInfo = &info;

    des_write[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    des_write[1].dstSet = *(set);
    des_write[1].dstBinding = 1;
    des_write[1].dstArrayElement = 0;
    des_write[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    des_write[1].descriptorCount = 1;
    des_write[1].pImageInfo = &image_info;
    vkUpdateDescriptorSets(vk_device, 2, des_write, 0, NULL);
  }
}
void pe_vk_create_descriptor_sets_skinned(PModel *model,
                                          PSkinnedMeshComponent *skinned) {

  VkDescriptorSetLayout layouts[4];

  ZERO(layouts);

  for (int i = 0; i < 4; i++) {
    layouts[i] = pe_vk_descriptor_set_layout_skinned;
  }

  array_init(&model->descriptor_sets, sizeof(VkDescriptorSet), 4);

  // resize because we need to allocate descriptor copy in array.data
  array_resize(&model->descriptor_sets, 4);

  // Allocation
  VkDescriptorSetAllocateInfo alloc_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = model->descriptor_pool,
      .descriptorSetCount = 4,
      .pSetLayouts = layouts};

  vkAllocateDescriptorSets(vk_device, &alloc_info, model->descriptor_sets.data);

  pe_vk_descriptor_update_skinned(model, skinned);
}

void pe_vk_create_descriptor_sets(PModel *model) {

  VkDescriptorSetLayout layouts[4];

  ZERO(layouts);

  for (int i = 0; i < 4; i++) {
    layouts[i] = pe_vk_descriptor_set_layout_with_texture;
  }

  array_init(&model->descriptor_sets, sizeof(VkDescriptorSet), 4);

  // resize because we need to allocate descriptor copy in array.data
  array_resize(&model->descriptor_sets, 4);

  // Allocation
  VkDescriptorSetAllocateInfo alloc_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = model->descriptor_pool,
      .descriptorSetCount = 4,
      .pSetLayouts = layouts};

  vkAllocateDescriptorSets(vk_device, &alloc_info, model->descriptor_sets.data);

  pe_vk_descriptor_update(model);
}
