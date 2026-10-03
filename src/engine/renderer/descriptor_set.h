#ifndef PE_VK_DESCRIPTOR_SET_H
#define PE_VK_DESCRIPTOR_SET_H

#include <engine/array.h>
#include <engine/macros.h>
#include <vulkan/vulkan.h>

#include <engine/model.h>
#include <engine/skeletal.h>

typedef struct PRenderTarget PRenderTarget;

extern VkPipelineLayout pe_vk_pipeline_layout;
extern VkPipelineLayout pe_vk_pipeline_layout_with_descriptors;
extern VkPipelineLayout pe_vk_pipeline_layout_skinned;
extern VkPipelineLayout pe_vk_pipeline_layout3;

extern VkDescriptorSetLayout pe_vk_descriptor_set_layout;
extern VkDescriptorSetLayout pe_vk_descriptor_set_layout_with_texture;
extern VkDescriptorSetLayout pe_vk_descriptor_set_layout_skinned;

void pe_vk_clean_descriptors_set();

void pe_vk_descriptor_pool_create(PModel *model, PRenderTarget *target);
void pe_vk_descriptor_with_image_update(PModel *model, PRenderTarget *target);

void pe_vk_create_descriptor_set_layout();
void pe_vk_create_descriptor_set_layout_with_texture();
void pe_vk_create_descriptor_set_layout_skinned();

void pe_vk_descriptor_update(PModel *model, PRenderTarget *target);
void pe_vk_create_descriptor_sets(PModel *model, VkDescriptorSetLayout layout,
                                  PRenderTarget *target);

//uniform buffer (binding 0), texture (binding 1) and skin's joint matrix
//storage buffer (binding 2) - call once, after pe_vk_skin_create_storage_buffers()
void pe_vk_descriptor_skinned_update(PModel *model, PSkin *skin,
                                     PRenderTarget *target);

//gives the primitives of a model that draw from its skin extra (texture_type
//PE_TEXTURE_TYPE_SKIN_EXTRA, see pe_model_set_active_geosets()) a texture of
//their own. takes over texture, which the model then frees with itself; one
//set before is freed here, once the gpu is idle. the descriptor sets of
//model must already be made and updated
void pe_vk_model_set_extra_texture(PModel *model, PSkin *skin,
                                   PTexture texture);

#endif
