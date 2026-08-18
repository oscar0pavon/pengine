#include "pipeline.h"
#include "descriptor_set.h"
#include "render_pass.h"
#include "shader_module.h"
#include "swap_chain.h"
#include "vk_vertex.h"
#include <engine/log.h>
#include <engine/macros.h>
#include "vulkan.h"
#include <stdbool.h>
#include <vulkan/vulkan_core.h>


Array pe_vk_pipeline_infos;
Array pe_graphics_pipelines;
    
//we use this for every create pipeline info
VkVertexInputBindingDescription input_binding_description;
VkPipelineColorBlendAttachmentState color_attachment;
VkPipelineVertexInputStateCreateInfo vertex_input_state;
VkPipelineViewportStateCreateInfo viewport_state;
VkPipelineDynamicStateCreateInfo dynamic_state;
VkPipelineRasterizationStateCreateInfo rasterization_state;
VkPipelineInputAssemblyStateCreateInfo input_assembly_state;
VkPipelineMultisampleStateCreateInfo multisample_state;
VkPipelineColorBlendStateCreateInfo color_blend_state;
VkVertexInputBindingDescription input_binding_description;
VkPipelineDepthStencilStateCreateInfo depth_stencil;


VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT,
                                  VK_DYNAMIC_STATE_SCISSOR};

void pe_vk_clean_layouts(){

  vkDestroyPipelineLayout(vk_device, pe_vk_pipeline_layout3, NULL);
  vkDestroyPipelineLayout(vk_device, pe_vk_pipeline_layout, NULL);
  vkDestroyPipelineLayout(vk_device, pe_vk_pipeline_layout_with_descriptors, NULL);
}

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

  VkPipelineVertexInputStateCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};

  if (attributes->has_attributes == true) {

    array_init(&attributes->attributes_descriptions,
               sizeof(VkVertexInputAttributeDescription), 6);

    input_binding_description =
        pe_vk_vertex_get_binding_description();
    info.vertexBindingDescriptionCount = 1;
    info.pVertexBindingDescriptions = &input_binding_description;

    pe_vk_vertex_get_attribute(attributes);
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

VkPipelineColorBlendStateCreateInfo pe_vk_pipeline_get_default_color_blend(bool transparency) {

  color_attachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  if(transparency == false)
    color_attachment.blendEnable = VK_FALSE;
  else{//transparency == true
    color_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_attachment.colorBlendOp = VK_BLEND_OP_ADD;

  }

  VkPipelineColorBlendStateCreateInfo color_blending = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .logicOpEnable = VK_FALSE,
      .attachmentCount = 1,
      .pAttachments = &color_attachment};

  if(transparency)
    color_blending.logicOp = VK_LOGIC_OP_COPY;

  return color_blending;
}

//you can fill pe_vk_pipeline_infos and then create 
//multiples piles at the time

void pe_vk_pipeline_create_pipelines() {

  array_resize(&pe_graphics_pipelines, pe_vk_pipeline_infos.count);
  VKVALID(vkCreateGraphicsPipelines(
              vk_device, VK_NULL_HANDLE, pe_vk_pipeline_infos.count,
              pe_vk_pipeline_infos.data, NULL, pe_graphics_pipelines.data),
          "Can't create pipelines");
  LOG("Created %i pipelines\n", pe_vk_pipeline_infos.count);
}

//create a default pipeline create info for 
//fill with shader and vertex input state
//and return the create info pointer to fill it
VkGraphicsPipelineCreateInfo* pe_vk_pipeline_create_info(){

  VkGraphicsPipelineCreateInfo pipeline_create_info = {

      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .stageCount = 2,//fragment and vertex stages
      .renderPass = pe_vk_render_pass, // created in pe_vk_create_render_pass()
      .pInputAssemblyState = &input_assembly_state,
      .pViewportState = &viewport_state,
      .pRasterizationState = &rasterization_state,
      .pMultisampleState = &multisample_state,
      .pColorBlendState = &color_blend_state,
      .pDynamicState = &dynamic_state,
      .pDepthStencilState = &depth_stencil,

      .subpass = 0};
  
  array_add(&pe_vk_pipeline_infos, &pipeline_create_info);
  return array_pop(&pe_vk_pipeline_infos);
}



void pe_vk_pipelines_init() {
  

  array_init(&pe_vk_pipeline_infos, sizeof(VkGraphicsPipelineCreateInfo),
             PE_VK_PIPELINES_MAX);

  array_init(&pe_graphics_pipelines, sizeof(VkPipeline), PE_VK_PIPELINES_MAX);

  
  depth_stencil = pe_vk_pipeline_get_default_depth_stencil();
  multisample_state = pe_vk_pipeline_get_default_multisample();
  input_assembly_state = pe_vk_pipeline_get_default_input_assembly();
  viewport_state = pe_vk_pipeline_get_default_viewport();
  dynamic_state = pe_vk_pipeline_get_default_dynamic_state();
  rasterization_state = pe_vk_pipeline_get_default_rasterization();

}
