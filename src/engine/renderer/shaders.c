#include "shaders.h"
#include "vulkan.h"
#include <engine/file_loader.h>
#include <engine/log.h>
#include <stdbool.h>
#include <vulkan/vulkan_core.h>
#include "shader_module.h"
#include "pipeline.h"

VkPipelineShaderStageCreateInfo pe_vk_shaders_stages_infos[2];

void pe_vk_clean_shader(PShader *shader) {
  if (shader->cleaned == false) {
    vkDestroyShaderModule(vk_device, shader->fragment, NULL);
    vkDestroyShaderModule(vk_device, shader->vertex, NULL);
    vkDestroyPipeline(vk_device, shader->pipeline, NULL);
    shader->cleaned = true;
  }
}

//load .spv built shaders
void pe_vk_shader_load(PCreateShaderInfo *info) {
  File new_file;
  ZERO(new_file);
  pe_file_openb(info->vertex_path, &new_file);

  File new_file2;
  ZERO(new_file2);
  pe_file_openb(info->fragment_path, &new_file2);


  VkShaderModule vertex_module = pe_vk_shader_module_create(&new_file);
  VkShaderModule fragment_module = pe_vk_shader_module_create(&new_file2);

  info->out_shader->vertex = vertex_module;
  info->out_shader->fragment= fragment_module;
  

  VkPipelineShaderStageCreateInfo info1;
  ZERO(info1);
  info1.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  info1.stage = VK_SHADER_STAGE_VERTEX_BIT;
  info1.module = vertex_module;
  info1.pName = "main";

  VkPipelineShaderStageCreateInfo info2;
  ZERO(info2);
  info2.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  info2.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  info2.module = fragment_module;
  info2.pName = "main";

  //we can reuse the global pe_vk_shaders_stages_infos for multiples shaders
  pe_vk_shaders_stages_infos[0] = info1;
  pe_vk_shaders_stages_infos[1] = info2;

  info->vk_create_info->pStages = pe_vk_shaders_stages_infos;

}

void pe_vk_create_shader(PCreateShaderInfo* info){
  info->out_shader->cleaned = false;

  info->vk_create_info = pe_vk_pipeline_create_info();
  
  if(info->transparency)
    color_blend_state = pe_vk_pipeline_get_default_color_blend(true);
  else
    color_blend_state = pe_vk_pipeline_get_default_color_blend(false);
  
  pe_vk_shader_load(info);


  // example can be have vertex position and UV or more
  PVertexAtrributes vertex_attributes = {.has_attributes = true,
                                         .position = true,
                                         .uv = true};

  ZERO(vertex_input_state);
  vertex_input_state =
      pe_vk_pipeline_get_default_vertex_input(&vertex_attributes);

  info->vk_create_info->pVertexInputState = &vertex_input_state;

  info->vk_create_info->layout = info->layout;

  int count = 1;
  VKVALID(vkCreateGraphicsPipelines(vk_device, VK_NULL_HANDLE, count,
                                    info->vk_create_info, NULL, &info->out_shader->pipeline),
          "Can't create pipeline");
}

//same as pe_vk_create_shader but the pipeline also reads a per instance
//buffer, so one draw can place many copies of the same mesh
void pe_vk_create_shader_instanced(PCreateShaderInfo *info) {
  info->out_shader->cleaned = false;

  info->vk_create_info = pe_vk_pipeline_create_info();

  if (info->transparency)
    color_blend_state = pe_vk_pipeline_get_default_color_blend(true);
  else
    color_blend_state = pe_vk_pipeline_get_default_color_blend(false);

  pe_vk_shader_load(info);

  ZERO(vertex_input_state);
  vertex_input_state = pe_vk_vertex_get_instanced_input();

  info->vk_create_info->pVertexInputState = &vertex_input_state;

  info->vk_create_info->layout = info->layout;

  int count = 1;
  VKVALID(vkCreateGraphicsPipelines(vk_device, VK_NULL_HANDLE, count,
                                    info->vk_create_info, NULL,
                                    &info->out_shader->pipeline),
          "Can't create instanced pipeline");
}
