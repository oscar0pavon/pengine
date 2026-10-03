#include "sky.h"

#include <engine/files.h>
#include <engine/macros.h>
#include <engine/renderer/descriptor_set.h>
#include <engine/renderer/pipeline.h>
#include <engine/renderer/uniform_buffer.h>
#include <engine/renderer/vulkan.h>

//the sky reads no vertex buffer, and neither tests nor writes depth
static void create_shader(PShader *shader) {
  VkPipelineVertexInputStateCreateInfo no_input = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};

  VkPipelineRasterizationStateCreateInfo rasterization = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = VK_CULL_MODE_NONE,
      .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
      .lineWidth = 1.0f};

  VkPipelineDepthStencilStateCreateInfo depth_stencil = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .depthTestEnable = VK_FALSE,
      .depthWriteEnable = VK_FALSE,
      .stencilTestEnable = VK_FALSE};

  PCreateShaderInfo info;
  ZERO(info);
  info.out_shader = shader;
  info.vertex_path = file_sky_hdr_vert_spv;
  info.fragment_path = file_sky_hdr_frag_spv;
  info.layout = pe_vk_pipeline_layout3;
  info.vertex_input = &no_input;
  info.rasterization = &rasterization;
  info.depth_stencil = &depth_stencil;
  pe_vk_create_shader(&info);
}

void pe_sky_create(PSky *sky, const PEnvironment *environment) {
  ZERO(*sky);
  sky->exposure = environment->block.settings[0];
  sky->carrier.texture = environment->panorama;

  create_shader(&sky->shader);

  PModel *carrier = &sky->carrier;
  pe_vk_create_uniform_buffers(carrier, &main_render_target);
  pe_vk_descriptor_pool_create(carrier, &main_render_target);
  pe_vk_create_descriptor_sets(carrier, pe_vk_descriptor_set_layout_with_texture,
                               &main_render_target);
  pe_vk_descriptor_with_image_update(carrier, &main_render_target);
}

void pe_sky_draw(PSky *sky, const PCamera *camera, VkCommandBuffer command,
                 u32 image_index) {
  PUniformBufferObject *ubo = &sky->carrier.uniform_buffer_object;
  glm_mat4_copy((vec4 *)camera->view, ubo->view);
  glm_mat4_copy((vec4 *)camera->projection, ubo->projection);
  ubo->color[0] = sky->exposure;
  pe_vk_send_uniform_buffer(&sky->carrier, image_index);

  VkDescriptorSet *set = array_get(&sky->carrier.descriptor_sets, image_index);
  vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    sky->shader.pipeline);
  vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pe_vk_pipeline_layout3, 0, 1, set, 0, NULL);
  vkCmdDraw(command, 3, 1, 0, 0);
}
