#ifndef PE_SHADERS_H
#define PE_SHADERS_H

#include <vulkan/vulkan_core.h>
#include <stdbool.h>

typedef struct PShader{
  bool cleaned;
  VkPipeline pipeline;
  VkShaderModule vertex;
  VkShaderModule fragment;
}PShader;

typedef struct PCreateShaderInfo{
    bool transparency;
    PShader *out_shader;
    const char* vertex_path;
    const char* fragment_path;
    VkPipelineLayout layout;
    VkGraphicsPipelineCreateInfo* vk_create_info;

    //how the vertex buffers are read. NULL means the position and uv of a PVertex
    const VkPipelineVertexInputStateCreateInfo* vertex_input;

    //cull mode and winding. NULL means the engine's default, which culls nothing
    const VkPipelineRasterizationStateCreateInfo* rasterization;

    //depth and stencil testing. NULL means the engine's default, which tests
    //and writes depth
    const VkPipelineDepthStencilStateCreateInfo* depth_stencil;

    //how the colour is combined with what is already there. NULL means the
    //engine's default, which overwrites it. the transparency flag above does
    //not enable blending, so a pipeline that needs it says so here
    const VkPipelineColorBlendStateCreateInfo* color_blend;
}PCreateShaderInfo;

void pe_vk_clean_shader(PShader *shader);

void pe_vk_create_shader(PCreateShaderInfo* info);

void pe_vk_create_shader_instanced(PCreateShaderInfo *info);

extern VkPipelineShaderStageCreateInfo shader_create_info[2];

#endif
