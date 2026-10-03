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
VkDescriptorSetLayout pe_vk_descriptor_set_layout_material;
VkDescriptorSetLayout pe_vk_descriptor_set_layout_environment;
VkPipelineLayout pe_vk_pipeline_layout_pbr;

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
//a part's material: the uniform buffer of the instance, then the colour, the
//metal and roughness and the normals of its surface
void pe_vk_create_descriptor_set_layout_material() {
  VkDescriptorSetLayoutBinding bindings[4];
  ZERO(bindings);
  bindings[0].binding = 0;
  bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  bindings[0].descriptorCount = 1;
  bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  for (int i = 1; i < 4; i++) {
    bindings[i].binding = i;
    bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[i].descriptorCount = 1;
    bindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  }

  VkDescriptorSetLayoutCreateInfo info;
  ZERO(info);
  info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  info.bindingCount = 4;
  info.pBindings = bindings;
  VKVALID(vkCreateDescriptorSetLayout(vk_device, &info, NULL,
                                      &pe_vk_descriptor_set_layout_material),
          "Can't create material descriptor set layout");
}

//the light of the world, shared by everything drawn in it: what its lights are
//and the panorama it is lit and reflected by
void pe_vk_create_descriptor_set_layout_environment() {
  VkDescriptorSetLayoutBinding bindings[2];
  ZERO(bindings);
  bindings[0].binding = 0;
  bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  bindings[0].descriptorCount = 1;
  bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  bindings[1].binding = 1;
  bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  bindings[1].descriptorCount = 1;
  bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutCreateInfo info;
  ZERO(info);
  info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  info.bindingCount = 2;
  info.pBindings = bindings;
  VKVALID(vkCreateDescriptorSetLayout(vk_device, &info, NULL,
                                      &pe_vk_descriptor_set_layout_environment),
          "Can't create environment descriptor set layout");
}

//set 0 is the material of the part and set 1 the environment. a part's own
//factors go in as a push constant, which is cheaper than a set to bind for each
void pe_vk_create_pipeline_layout_pbr() {
  VkDescriptorSetLayout set_layouts[] = {
      pe_vk_descriptor_set_layout_material,
      pe_vk_descriptor_set_layout_environment};

  VkPushConstantRange push = {.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                              .offset = 0,
                              .size = sizeof(PPartMaterial)};

  VkPipelineLayoutCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 2,
      .pSetLayouts = set_layouts,
      .pushConstantRangeCount = 1,
      .pPushConstantRanges = &push};
  VKVALID(vkCreatePipelineLayout(vk_device, &info, NULL,
                                 &pe_vk_pipeline_layout_pbr),
          "Can't create pbr pipeline layout");
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

static void pe_vk_descriptor_skinned_write(PModel *model, PSkin *skin,
                                           Array *descriptor_sets,
                                           PTexture *texture) {

  u32 count = pe_vk_targets_max_images_count();

  for (u32 i = 0; i < count; i++) {

    VkBuffer *uniform_buffer = array_get(&model->uniform_buffers, i);
    VkDescriptorBufferInfo uniform_info = {
        .buffer = *uniform_buffer,
        .offset = 0,
        .range = sizeof(PUniformBufferObject)};

    VkDescriptorImageInfo image_info = {
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .imageView = texture->image_view,
        .sampler = texture->sampler};

    VkBuffer *storage_buffer = array_get(&skin->shader_storage_buffers, i);
    VkDescriptorBufferInfo storage_info = {
        .buffer = *storage_buffer,
        .offset = 0,
        .range = sizeof(skin->node_uniform.joints_matrix)};

    VkDescriptorSet *descriptor_set = array_get(descriptor_sets, i);

    VkWriteDescriptorSet des_write[3] = {
        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = *descriptor_set,
         .dstBinding = 0,
         .dstArrayElement = 0,
         .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
         .descriptorCount = 1,
         .pBufferInfo = &uniform_info},
        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = *descriptor_set,
         .dstBinding = 1,
         .dstArrayElement = 0,
         .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         .descriptorCount = 1,
         .pImageInfo = &image_info},
        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = *descriptor_set,
         .dstBinding = 2,
         .dstArrayElement = 0,
         .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
         .descriptorCount = 1,
         .pBufferInfo = &storage_info},
    };

    vkUpdateDescriptorSets(vk_device, 3, des_write, 0, NULL);
  }
}

void pe_vk_descriptor_skinned_update(PModel *model, PSkin *skin,
                                     PRenderTarget *target) {
  pe_vk_descriptor_skinned_write(model, skin, &model->descriptor_sets,
                                 &model->texture);
}

void pe_vk_model_set_extra_texture(PModel *model, PSkin *skin,
                                   PTexture texture) {
  u32 count = pe_vk_targets_max_images_count();

  if (!model->has_extra_texture) {
    VkDescriptorPoolSize pool_size[3] = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, count},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, count},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, count}};
    VkDescriptorPoolCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount = 3,
        .pPoolSizes = pool_size,
        .maxSets = count};
    VKVALID(vkCreateDescriptorPool(vk_device, &info, NULL,
                                   &model->extra_descriptor_pool),
            "Can't create descriptor pool");

    VkDescriptorSetLayout layouts[count];
    for (u32 i = 0; i < count; i++)
      layouts[i] = pe_vk_descriptor_set_layout_skinned;

    array_init(&model->extra_descriptor_sets, sizeof(VkDescriptorSet), count);
    array_resize(&model->extra_descriptor_sets, count);

    VkDescriptorSetAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = model->extra_descriptor_pool,
        .descriptorSetCount = count,
        .pSetLayouts = layouts};
    vkAllocateDescriptorSets(vk_device, &alloc_info,
                             model->extra_descriptor_sets.data);
    model->has_extra_texture = true;
  } else {
    vkDeviceWaitIdle(vk_device);
    pe_vk_clean_image(&model->extra_texture);
  }

  model->extra_texture = texture;
  pe_vk_descriptor_skinned_write(model, skin, &model->extra_descriptor_sets,
                                 &model->extra_texture);
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

//the sets a model of several parts is drawn with, one for each part and swap
//chain image, each pointing at the instance's uniform buffer and the part's
//texture. the plain sets of the model are not used to draw it. call after the
//uniform buffers are made
void pe_vk_model_parts_create_descriptors(PModel *model) {
  u32 images = pe_vk_targets_max_images_count();
  u32 sets = model->parts.count * images;

  VkDescriptorPoolSize pool_size[2];
  ZERO(pool_size);
  pool_size[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  pool_size[0].descriptorCount = sets;
  pool_size[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  pool_size[1].descriptorCount = sets * 3;

  VkDescriptorPoolCreateInfo pool_info;
  ZERO(pool_info);
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.poolSizeCount = 2;
  pool_info.pPoolSizes = pool_size;
  pool_info.maxSets = sets;
  VKVALID(vkCreateDescriptorPool(vk_device, &pool_info, NULL,
                                 &model->part_descriptor_pool),
          "Can't create descriptor pool of parts");

  VkDescriptorSetLayout layouts[sets];
  for (u32 i = 0; i < sets; i++)
    layouts[i] = pe_vk_descriptor_set_layout_material;

  array_init(&model->part_descriptor_sets, sizeof(VkDescriptorSet), sets);
  array_resize(&model->part_descriptor_sets, sets);

  VkDescriptorSetAllocateInfo alloc_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = model->part_descriptor_pool,
      .descriptorSetCount = sets,
      .pSetLayouts = layouts};
  VKVALID(vkAllocateDescriptorSets(vk_device, &alloc_info,
                                   model->part_descriptor_sets.data),
          "Can't allocate descriptor sets of parts");

  for (u32 part_index = 0; part_index < model->parts.count; part_index++) {
    PModelPart *part = array_get(&model->parts, part_index);

    for (u32 i = 0; i < images; i++) {
      VkBuffer *buffer = array_get(&model->uniform_buffers, i);
      VkDescriptorBufferInfo buffer_info = {
          .buffer = *buffer, .offset = 0, .range = sizeof(PUniformBufferObject)};
      const PTexture *textures[3] = {&part->texture, &part->metallic_roughness,
                                     &part->normal};
      VkDescriptorImageInfo image_info[3];
      for (int t = 0; t < 3; t++) {
        image_info[t].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info[t].imageView = textures[t]->image_view;
        image_info[t].sampler = textures[t]->sampler;
      }

      VkDescriptorSet *set =
          array_get(&model->part_descriptor_sets, part_index * images + i);
      VkWriteDescriptorSet writes[4];
      ZERO(writes);
      writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      writes[0].dstSet = *set;
      writes[0].dstBinding = 0;
      writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      writes[0].descriptorCount = 1;
      writes[0].pBufferInfo = &buffer_info;
      for (int t = 0; t < 3; t++) {
        writes[t + 1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[t + 1].dstSet = *set;
        writes[t + 1].dstBinding = t + 1;
        writes[t + 1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[t + 1].descriptorCount = 1;
        writes[t + 1].pImageInfo = &image_info[t];
      }
      vkUpdateDescriptorSets(vk_device, 4, writes, 0, NULL);
    }
  }
}
