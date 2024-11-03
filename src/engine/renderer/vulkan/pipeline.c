#include "pipeline.h"
#include "descriptor_set.h"
#include "render_pass.h"
#include "shader_module.h"
#include "swap_chain.h"
#include "vk_vertex.h"
#include <engine/log.h>
#include <engine/macros.h>
#include <engine/renderer/vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <engine/files.h>

VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT,
                                  VK_DYNAMIC_STATE_SCISSOR};

void pe_vk_pipeline_create_layout(bool use_descriptor, VkPipelineLayout *layout,
                                  VkDescriptorSetLayout *set_layout) {

  VkPipelineLayoutCreateInfo info;
  ZERO(info);
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  info.pNext = NULL;
  info.flags = 0;
  info.setLayoutCount = 0;
  info.pSetLayouts = NULL;

  if (use_descriptor == true) {
    info.setLayoutCount = 1;
    info.pSetLayouts = set_layout;
  }

  VKVALID(vkCreatePipelineLayout(vk_device, &info, NULL, layout),
          "Can't create Pipeline Layout");
}

typedef struct PPipelineInfo {
  VkPipelineVertexInputStateCreateInfo vertex_input_state;
  VkPipelineViewportStateCreateInfo viewport_state;
  VkPipelineDynamicStateCreateInfo dynamic_state;
  VkPipelineRasterizationStateCreateInfo rasterization_state;
  VkPipelineInputAssemblyStateCreateInfo input_assembly_state;
  VkPipelineMultisampleStateCreateInfo multisample_state;
  VkPipelineColorBlendAttachmentState color_attachment;
  VkPipelineColorBlendStateCreateInfo color_blend_state;
  VkVertexInputBindingDescription input_binding_description;
  VkPipelineDepthStencilStateCreateInfo depth_stencil;
  PVertexAtrributes attributes;
} PPipelineInfo;

PPipelineInfo pe_vk_main_pipeline_info;

VkPipelineDepthStencilStateCreateInfo
pe_vk_pipeline_get_default_depth_stencil() {
  VkPipelineDepthStencilStateCreateInfo info;
  ZERO(info);
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  info.depthTestEnable = VK_TRUE;
  info.depthWriteEnable = VK_TRUE;
  info.depthCompareOp = VK_COMPARE_OP_LESS;
  info.depthBoundsTestEnable = VK_FALSE;
  info.minDepthBounds = 0.0f;
  info.maxDepthBounds = 1.0f;
  info.stencilTestEnable = VK_TRUE;

  return info;
}

VkPipelineDynamicStateCreateInfo pe_vk_pipeline_get_default_dynamic_state() {

  VkPipelineDynamicStateCreateInfo dynamicStateInfo;
  ZERO(dynamicStateInfo);
  dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicStateInfo.dynamicStateCount = 2;
  dynamicStateInfo.pDynamicStates = dynamicStates;
  return dynamicStateInfo;
}

VkPipelineVertexInputStateCreateInfo
pe_vk_pipeline_get_default_vertex_input(PVertexAtrributes *attributes) {

  VkPipelineVertexInputStateCreateInfo info;
  ZERO(info);
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  info.vertexBindingDescriptionCount = 0;
  info.pVertexBindingDescriptions = NULL;
  info.vertexAttributeDescriptionCount = 0;
  info.pVertexAttributeDescriptions = NULL;

  if (attributes->has_attributes == true) {

    array_init(&attributes->attributes_descriptions,
               sizeof(VkVertexInputAttributeDescription), 6);

    pe_vk_main_pipeline_info.input_binding_description =
        pe_vk_vertex_get_binding_description();

    pe_vk_vertex_get_attribute(attributes);
    info.vertexBindingDescriptionCount = 1;
    info.pVertexBindingDescriptions =
        &pe_vk_main_pipeline_info.input_binding_description;
    info.vertexAttributeDescriptionCount =
        attributes->attributes_descriptions.count;
    info.pVertexAttributeDescriptions =
        attributes->attributes_descriptions.data;
  }
  return info;
}

VkPipelineViewportStateCreateInfo pe_vk_pipeline_get_default_viewport() {

  VkViewport viewport;
  ZERO(viewport);
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)pe_vk_swch_extent.width;
  viewport.height = (float)pe_vk_swch_extent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkOffset2D offset;
  ZERO(offset);
  offset.x = 0;
  offset.y = 0;

  VkRect2D scissor;
  ZERO(scissor);
  scissor.offset = offset;
  scissor.extent = pe_vk_swch_extent;

  VkPipelineViewportStateCreateInfo viewportState;
  ZERO(viewportState);
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

  return viewportState;
}

VkPipelineRasterizationStateCreateInfo
pe_vk_pipeline_get_default_rasterization() {

  // rasterizer
  VkPipelineRasterizationStateCreateInfo rasterizer;
  ZERO(rasterizer);
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_NONE;
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;
  return rasterizer;
}

VkPipelineInputAssemblyStateCreateInfo
pe_vk_pipeline_get_default_input_assembly() {

  VkPipelineInputAssemblyStateCreateInfo inputAssembly;
  ZERO(inputAssembly);
  inputAssembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;
  return inputAssembly;
}

VkPipelineMultisampleStateCreateInfo pe_vk_pipeline_get_default_multisample() {

  // multisampling
  VkPipelineMultisampleStateCreateInfo multisampling;
  ZERO(multisampling);
  multisampling.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = pe_vk_msaa_samples;
  return multisampling;
}

VkPipelineColorBlendStateCreateInfo pe_vk_pipeline_get_default_color_blend() {

  pe_vk_main_pipeline_info.color_attachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  pe_vk_main_pipeline_info.color_attachment.blendEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo colorBlending;
  ZERO(colorBlending);
  colorBlending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &pe_vk_main_pipeline_info.color_attachment;

  return colorBlending;
}

void pe_vk_pipeline_create_pipelines() {

  array_resize(&pe_graphics_pipelines, pe_vk_pipeline_infos.count);
  VKVALID(vkCreateGraphicsPipelines(
              vk_device, VK_NULL_HANDLE, pe_vk_pipeline_infos.count,
              pe_vk_pipeline_infos.data, NULL, pe_graphics_pipelines.data),
          "Can't create pipelines");
  LOG("Created %i pipelines\n", pe_vk_pipeline_infos.count);
}

void pe_vk_pipelines_init() {
  ZERO(pe_vk_main_pipeline_info);
  array_init(&pe_vk_pipeline_infos, sizeof(VkGraphicsPipelineCreateInfo),
             PE_VK_PIPELINES_MAX);
  array_init(&pe_graphics_pipelines, sizeof(VkPipeline), PE_VK_PIPELINES_MAX);
  pe_vk_main_pipeline_info.attributes.has_attributes = false;
  pe_vk_main_pipeline_info.vertex_input_state =
      pe_vk_pipeline_get_default_vertex_input(
          &pe_vk_main_pipeline_info.attributes);
  pe_vk_main_pipeline_info.rasterization_state =
      pe_vk_pipeline_get_default_rasterization();
  pe_vk_main_pipeline_info.dynamic_state =
      pe_vk_pipeline_get_default_dynamic_state();
  pe_vk_main_pipeline_info.viewport_state =
      pe_vk_pipeline_get_default_viewport();
  pe_vk_main_pipeline_info.input_assembly_state =
      pe_vk_pipeline_get_default_input_assembly();
  pe_vk_main_pipeline_info.multisample_state =
      pe_vk_pipeline_get_default_multisample();
  pe_vk_main_pipeline_info.color_blend_state =
      pe_vk_pipeline_get_default_color_blend();
  pe_vk_main_pipeline_info.depth_stencil =
      pe_vk_pipeline_get_default_depth_stencil();

  VkGraphicsPipelineCreateInfo base_pipeline_info = {

      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .stageCount = 2,
      //.pStages = red_shader, // created in pe_vk_shader_load()
      .layout =
          pe_vk_pipeline_layout, // created in pe_vk_pipeline_create_layout()
      .renderPass = pe_vk_render_pass, // created in pe_vk_create_render_pass()
      .pVertexInputState = &pe_vk_main_pipeline_info.vertex_input_state,
      .pInputAssemblyState = &pe_vk_main_pipeline_info.input_assembly_state,
      .pViewportState = &pe_vk_main_pipeline_info.viewport_state,
      .pRasterizationState = &pe_vk_main_pipeline_info.rasterization_state,
      .pMultisampleState = &pe_vk_main_pipeline_info.multisample_state,
      .pColorBlendState = &pe_vk_main_pipeline_info.color_blend_state,
      .pDynamicState = &pe_vk_main_pipeline_info.dynamic_state,
      .pDepthStencilState = &pe_vk_main_pipeline_info.depth_stencil,

      .subpass = 0};

  VkPipelineShaderStageCreateInfo red_shader[2];
  ZERO(red_shader);
  pe_vk_shader_load(red_shader,
                    file_other_vert_spv,
                    file_blue_frag_spv);
  base_pipeline_info.pStages = red_shader;
  array_add(&pe_vk_pipeline_infos, &base_pipeline_info);

  // red triangle
  VkPipelineShaderStageCreateInfo blue_shader[2];
  pe_vk_shader_load(blue_shader,
                    file_vert_spv,
                    file_frag_spv);
  base_pipeline_info.pStages = blue_shader;
  array_add(&pe_vk_pipeline_infos, &base_pipeline_info);

  // vertex buffer model
  VkPipelineShaderStageCreateInfo in_position[2];
  pe_vk_shader_load(in_position,
                    file_in_position_spv,
                    file_frag_spv);
  base_pipeline_info.pStages = in_position;
  PPipelineInfo in_position_pipeline_info;
  ZERO(in_position_pipeline_info);
  in_position_pipeline_info.attributes.has_attributes = true;
  in_position_pipeline_info.attributes.position = true;
  in_position_pipeline_info.vertex_input_state =
      pe_vk_pipeline_get_default_vertex_input(
          &in_position_pipeline_info.attributes);
  base_pipeline_info.pVertexInputState =
      &in_position_pipeline_info.vertex_input_state;
  array_add(&pe_vk_pipeline_infos, &base_pipeline_info);

  VkPipelineShaderStageCreateInfo uniform[2];
  pe_vk_shader_load(uniform,
                    file_diffuse_vert_spv,
                    file_diffuse_frag_spv);
  base_pipeline_info.pStages = uniform;
  PPipelineInfo uniform_pipeline_info;
  ZERO(uniform_pipeline_info);
  uniform_pipeline_info.attributes.has_attributes = true;
  uniform_pipeline_info.attributes.position = true;
  uniform_pipeline_info.attributes.color = true;
  uniform_pipeline_info.attributes.normal = true;
  uniform_pipeline_info.attributes.uv = true;
  uniform_pipeline_info.vertex_input_state =
      pe_vk_pipeline_get_default_vertex_input(
          &uniform_pipeline_info.attributes);
  base_pipeline_info.pVertexInputState =
      &uniform_pipeline_info.vertex_input_state;
  base_pipeline_info.layout = pe_vk_pipeline_layout_with_descriptors;
  array_add(&pe_vk_pipeline_infos, &base_pipeline_info);

  // #################### Skinned
  //
  VkPipelineShaderStageCreateInfo skinned[2];
  pe_vk_shader_load(skinned,
                    file_skinned_spv,
                    file_diffuse_frag_spv);
  base_pipeline_info.pStages = skinned;
  PPipelineInfo skinned_pipeline_info;
  ZERO(skinned_pipeline_info);
  skinned_pipeline_info.attributes.has_attributes = true;
  skinned_pipeline_info.attributes.position = true;
  skinned_pipeline_info.attributes.color = true;
  skinned_pipeline_info.attributes.normal = true;
  skinned_pipeline_info.attributes.uv = true;
  skinned_pipeline_info.attributes.weight = true;
  skinned_pipeline_info.attributes.joint = true;

  skinned_pipeline_info.vertex_input_state =
      pe_vk_pipeline_get_default_vertex_input(
          &skinned_pipeline_info.attributes);

  base_pipeline_info.pVertexInputState =
      &skinned_pipeline_info.vertex_input_state;
  base_pipeline_info.layout = pe_vk_pipeline_layout_skinned;
  array_add(&pe_vk_pipeline_infos, &base_pipeline_info);

  // ####################################################
  // ############## Create pipelines ##################
  pe_vk_pipeline_create_pipelines();

  VkPipeline *triangle_pipeline = array_get(&pe_graphics_pipelines, 0);
  pe_vk_pipeline = *(triangle_pipeline);
}
