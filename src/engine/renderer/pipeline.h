#include "shaders.h"
#include "vulkan.h"
#include <engine/array.h>
#include <vulkan/vulkan_core.h>

#include "shaders.h"
#include "vk_vertex.h"

#define PE_VK_PIPELINES_MAX 50

extern VkPipeline pe_vk_pipeline;

extern Array pe_vk_pipeline_infos;
extern Array pe_graphics_pipelines;

extern VkPipelineColorBlendStateCreateInfo color_blend_state;
extern VkPipelineVertexInputStateCreateInfo vertex_input_state;

VkGraphicsPipelineCreateInfo *pe_vk_pipeline_create_info();

void pe_vk_pipelines_init();

void pe_vk_clean_layouts();

void pe_vk_pipeline_create_layout(bool use_descriptor, VkPipelineLayout *layout,
                                  VkDescriptorSetLayout *set_layout);

VkPipelineColorBlendStateCreateInfo
pe_vk_pipeline_get_default_color_blend(bool transparency);

VkPipelineVertexInputStateCreateInfo
pe_vk_pipeline_get_default_vertex_input(PVertexAtrributes *attributes);

void pe_vk_create_shader(PCreateShaderInfo *);

void pe_vk_create_shader_instanced(PCreateShaderInfo *);
