#ifndef DESCRIPTOR_SET_H
#define DESCRIPTOR_SET_H

#include <engine/array.h>
#include <engine/macros.h>
#include <vulkan/vulkan.h>

#include <engine/model.h>


extern VkPipelineLayout pe_vk_pipeline_layout;
extern VkPipelineLayout pe_vk_pipeline_layout_with_descriptors;
extern VkPipelineLayout pe_vk_pipeline_layout_skinned;
extern VkPipelineLayout pe_vk_pipeline_layout3;

extern VkDescriptorSetLayout pe_vk_descriptor_set_layout;
extern VkDescriptorSetLayout pe_vk_descriptor_set_layout_with_texture;
extern VkDescriptorSetLayout pe_vk_descriptor_set_layout_skinned;

void pe_vk_clean_descriptors_set();

void pe_vk_descriptor_pool_create(PModel *model);
void pe_vk_descriptor_with_image_update(PModel *model);

void pe_vk_create_descriptor_set_layout();
void pe_vk_create_descriptor_set_layout_with_texture();
void pe_vk_create_descriptor_set_layout_skinned();

void pe_vk_descriptor_update(PModel *model);
void pe_vk_create_descriptor_sets(PModel *model, VkDescriptorSetLayout layout);

#endif
