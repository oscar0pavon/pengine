#include "descriptor_set.h"

#include "swap_chain.h"
#include "vulkan.h"
#include "vk_images.h"
#include <engine/array.h>

#include <vulkan/vulkan_core.h>

VkPipelineLayout pe_vk_pipeline_layout;
VkPipelineLayout pe_vk_pipeline_layout_with_descriptors;

VkPipelineLayout pe_vk_pipeline_layout3;

VkPipelineLayout pe_vk_pipeline_layout_skinned;

VkDescriptorSetLayout pe_vk_descriptor_set_layout;
VkDescriptorSetLayout pe_vk_descriptor_set_layout_with_texture;
VkDescriptorSetLayout pe_vk_descriptor_set_layout_skinned;

void pe_vk_clean_descriptors_set(){
  vkDestroyDescriptorSetLayout(vk_device, pe_vk_descriptor_set_layout_with_texture, NULL);
  vkDestroyDescriptorSetLayout(vk_device, pe_vk_descriptor_set_layout, NULL);
}

void pe_vk_descriptor_pool_create(PModel *model, PRenderTarget *target) {
  //INFO sized like the uniform buffer arrays in uniform_buffer.c: a model's
  //descriptor sets are shared across every target it draws on, so the pool
  //has to cover whichever target has the most swap chain images
  u32 count = pe_vk_targets_max_images_count();

  VkDescriptorPoolSize pool_size[3];
  ZERO(pool_size);
  pool_size[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  pool_size[0].descriptorCount = count;
  pool_size[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  pool_size[1].descriptorCount = count;
  pool_size[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  pool_size[2].descriptorCount = count;

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
void pe_vk_descriptor_with_image_update(PModel *model, PRenderTarget *target) {

  //INFO writes every slot the arrays were sized for
  //(pe_vk_targets_max_images_count()), not just target->images_count -
  //otherwise a target with more swap chain images than the one this was
  //called with would draw with descriptor sets that were allocated but never
  //pointed at a uniform buffer
  u32 count = pe_vk_targets_max_images_count();

  for (int i = 0; i < count; i++) {

    VkBuffer *buffer = array_get(&model->uniform_buffers, i);
    VkDescriptorBufferInfo info = {.buffer = *buffer,
                                   .offset = 0,
                                   .range = sizeof(PUniformBufferObject)};

    VkDescriptorImageInfo image_info = {
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .imageView = model->texture.image_view,
        .sampler = model->texture.sampler};

    VkDescriptorSet *descriptor_set = array_get(&model->descriptor_sets, i);

    VkWriteDescriptorSet des_write[2] = {
        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = *descriptor_set,
         .dstBinding = 0,
         .dstArrayElement = 0,
         .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
         .descriptorCount = 1,
         .pBufferInfo = &info},
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = *descriptor_set,
         .dstBinding = 1,// INFO this is the binding to the shader input
         .dstArrayElement = 0,
         .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         .descriptorCount = 1,
         .pImageInfo = &image_info},
    };

    vkUpdateDescriptorSets(vk_device, 2, des_write, 0, NULL);//2  descriptor write
  }
}

// INFO
// here is where you send uniform buffer with MVP matrix
void pe_vk_descriptor_update(PModel *model, PRenderTarget *target) {

  u32 count = pe_vk_targets_max_images_count();

  for (int i = 0; i < count; i++) {

    VkBuffer *buffer = array_get(&model->uniform_buffers, i);
    VkDescriptorBufferInfo info = {
        .buffer = *buffer, .offset = 0, .range = sizeof(PUniformBufferObject)};

    VkDescriptorSet *descriptor_set = array_get(&model->descriptor_sets, i);

    VkWriteDescriptorSet des_write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = *descriptor_set,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .pBufferInfo = &info};
    vkUpdateDescriptorSets(vk_device, 1, &des_write, 0, NULL);
  }
}

void pe_vk_create_descriptor_sets(PModel *model, VkDescriptorSetLayout layout,
                                  PRenderTarget *target) {

  //INFO same reasoning as pe_vk_descriptor_pool_create(): this array is
  //shared across every target the model draws on, and pe_vk_descriptor_update
  ///pe_vk_descriptor_with_image_update() below index it up to whichever
  //target they're called with, not just this one
  u32 count = pe_vk_targets_max_images_count();

  VkDescriptorSetLayout layouts[count];

  ZERO(layouts);

  for (int i = 0; i < count; i++) {
    //layouts[i] = pe_vk_descriptor_set_layout_with_texture;
    layouts[i] = layout;
  }

  array_init(&model->descriptor_sets, sizeof(VkDescriptorSet), count);

  // resize because we need to allocate descriptor copy in array.data
  array_resize(&model->descriptor_sets, count);

  // Allocation
  VkDescriptorSetAllocateInfo alloc_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = model->descriptor_pool,
      .descriptorSetCount = count,
      .pSetLayouts = layouts};

  vkAllocateDescriptorSets(vk_device, &alloc_info, model->descriptor_sets.data);

}
