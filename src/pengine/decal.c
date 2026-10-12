#include "decal.h"

#include <stdlib.h>
#include <string.h>

#include "file_loader.h"
#include "files.h"
#include "utils.h"

#include "renderer/shader_module.h"
#include "renderer/vk_buffer.h"
#include "renderer/vulkan.h"

#define DECAL_MAX_VERTICES 8192
#define DECAL_MAX_RUNS 64
#define DECAL_BLEND_COUNT 2

typedef struct DecalRun {
  PUiImage *image;
  PUiBlend blend;
  uint32_t first;
  uint32_t count;
} DecalRun;

typedef struct DecalFrame {
  PBuffer buffer;
  PDecalVertex *mapped;
} DecalFrame;

static VkPipelineLayout pipeline_layout;
static VkPipeline pipelines[DECAL_BLEND_COUNT];
static DecalFrame *frames;
static uint32_t frame_count;
static DecalRun runs[DECAL_MAX_RUNS];
static uint32_t run_count;
static uint32_t vertex_count;
static uint32_t current_frame;
static VkCommandBuffer current_cmd;
static mat4 current_view_projection;
static bool batching;
static bool ready;

static bool create_layout() {
  VkDescriptorSetLayout set_layout = pe_ui_image_set_layout();
  VkPushConstantRange push_constant = {.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                                       .size = sizeof(mat4)};
  VkPipelineLayoutCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 1,
      .pSetLayouts = &set_layout,
      .pushConstantRangeCount = 1,
      .pPushConstantRanges = &push_constant};
  return vkCreatePipelineLayout(vk_device, &info, NULL, &pipeline_layout) == VK_SUCCESS;
}

static VkPipelineColorBlendAttachmentState blend_state(PUiBlend blend) {
  return (VkPipelineColorBlendAttachmentState){
      .blendEnable = VK_TRUE,
      .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
      .dstColorBlendFactor = blend == PE_UI_BLEND_ADD ? VK_BLEND_FACTOR_ONE
                                                       : VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      .colorBlendOp = VK_BLEND_OP_ADD,
      .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
      .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      .alphaBlendOp = VK_BLEND_OP_ADD,
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};
}

static bool create_pipelines() {
  File vertex_file, fragment_file;
  if (load_file(file_decal_vert_spv, &vertex_file) == -1 ||
      load_file(file_ui_frag_spv, &fragment_file) == -1)
    return false;

  VkShaderModule vertex = pe_vk_shader_module_create(&vertex_file);
  VkShaderModule fragment = pe_vk_shader_module_create(&fragment_file);
  close_file(&vertex_file);
  close_file(&fragment_file);

  VkPipelineShaderStageCreateInfo stages[2] = {
      {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
       .stage = VK_SHADER_STAGE_VERTEX_BIT,
       .module = vertex,
       .pName = "main"},
      {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
       .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
       .module = fragment,
       .pName = "main"}};

  VkVertexInputBindingDescription binding = {.stride = sizeof(PDecalVertex),
                                             .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};
  VkVertexInputAttributeDescription attributes[3] = {
      {.location = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(PDecalVertex, position)},
      {.location = 1, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(PDecalVertex, u)},
      {.location = 2, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(PDecalVertex, color)}};
  VkPipelineVertexInputStateCreateInfo vertex_input = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount = 1,
      .pVertexBindingDescriptions = &binding,
      .vertexAttributeDescriptionCount = 3,
      .pVertexAttributeDescriptions = attributes};

  VkPipelineInputAssemblyStateCreateInfo input_assembly = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
  VkPipelineViewportStateCreateInfo viewport = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .scissorCount = 1};
  VkPipelineRasterizationStateCreateInfo rasterizer = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = VK_CULL_MODE_NONE,
      .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
      .depthBiasEnable = VK_TRUE,
      .depthBiasConstantFactor = -2.f,
      .depthBiasSlopeFactor = -2.f,
      .lineWidth = 1.f};
  VkPipelineMultisampleStateCreateInfo multisample = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = pe_vk_msaa_samples};
  VkPipelineDepthStencilStateCreateInfo depth = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .depthTestEnable = VK_TRUE,
      .depthWriteEnable = VK_FALSE,
      .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL};
  VkDynamicState dynamic_states[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamic = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .dynamicStateCount = 2,
      .pDynamicStates = dynamic_states};

  bool ok = vertex && fragment;
  for (int blend = 0; ok && blend < DECAL_BLEND_COUNT; blend++) {
    VkPipelineColorBlendAttachmentState attachment = blend_state(blend);
    VkPipelineColorBlendStateCreateInfo color_blend = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &attachment};
    VkGraphicsPipelineCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = stages,
        .pVertexInputState = &vertex_input,
        .pInputAssemblyState = &input_assembly,
        .pViewportState = &viewport,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisample,
        .pDepthStencilState = &depth,
        .pColorBlendState = &color_blend,
        .pDynamicState = &dynamic,
        .layout = pipeline_layout,
        .renderPass = pe_vk_render_pass};
    ok = vkCreateGraphicsPipelines(vk_device, VK_NULL_HANDLE, 1, &info, NULL, &pipelines[blend]) ==
         VK_SUCCESS;
  }

  if (vertex)
    vkDestroyShaderModule(vk_device, vertex, NULL);
  if (fragment)
    vkDestroyShaderModule(vk_device, fragment, NULL);
  return ok;
}

static bool create_frames(uint32_t count) {
  frames = calloc(count, sizeof(DecalFrame));
  if (!frames)
    return false;
  frame_count = count;

  size_t size = sizeof(PDecalVertex) * DECAL_MAX_VERTICES;
  void *zeros = calloc(1, size);
  for (uint32_t i = 0; i < count; i++) {
    frames[i].buffer = pe_vk_create_buffer(size, zeros, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    vkMapMemory(vk_device, frames[i].buffer.memory, 0, size, 0, (void **)&frames[i].mapped);
  }
  free(zeros);
  return true;
}

int pe_decal_init() {
  ready = create_layout() && create_pipelines() && create_frames(main_render_target.images_count);
  return ready;
}

void pe_decal_shutdown() {
  if (!ready)
    return;

  vkDeviceWaitIdle(vk_device);
  for (uint32_t i = 0; i < frame_count; i++) {
    vkUnmapMemory(vk_device, frames[i].buffer.memory);
    pe_vk_destroy_buffer(&frames[i].buffer);
  }
  free(frames);
  frames = NULL;

  for (int blend = 0; blend < DECAL_BLEND_COUNT; blend++)
    vkDestroyPipeline(vk_device, pipelines[blend], NULL);
  vkDestroyPipelineLayout(vk_device, pipeline_layout, NULL);
  ready = false;
}

void pe_decal_begin(VkCommandBuffer cmd, uint32_t image_index, const mat4 view_projection) {
  if (!ready)
    return;

  current_cmd = cmd;
  current_frame = image_index;
  glm_mat4_copy((vec4 *)view_projection, current_view_projection);
  vertex_count = 0;
  run_count = 0;
  batching = true;
}

void pe_decal_triangles(PUiImage *image, PUiBlend blend, const PDecalVertex *vertices, uint32_t count) {
  if (!ready || !batching || vertex_count + count > DECAL_MAX_VERTICES)
    return;

  memcpy(&frames[current_frame].mapped[vertex_count], vertices, count * sizeof(PDecalVertex));

  DecalRun *last = run_count ? &runs[run_count - 1] : NULL;
  if (last && last->image == image && last->blend == blend) {
    last->count += count;
  } else if (run_count < DECAL_MAX_RUNS) {
    runs[run_count++] = (DecalRun){image, blend, vertex_count, count};
  } else {
    return;
  }
  vertex_count += count;
}

void pe_decal_end() {
  if (!ready || !batching)
    return;
  batching = false;
  if (run_count == 0)
    return;

  VkViewport viewport = {.width = main_render_target.extent.width,
                         .height = main_render_target.extent.height,
                         .maxDepth = 1.f};
  VkRect2D scissor = {.extent = main_render_target.extent};
  VkDeviceSize offset = 0;

  vkCmdSetViewport(current_cmd, 0, 1, &viewport);
  vkCmdSetScissor(current_cmd, 0, 1, &scissor);
  vkCmdPushConstants(current_cmd, pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4),
                     current_view_projection);
  vkCmdBindVertexBuffers(current_cmd, 0, 1, &frames[current_frame].buffer.buffer, &offset);

  int bound_blend = -1;
  for (uint32_t i = 0; i < run_count; i++) {
    DecalRun *run = &runs[i];
    if ((int)run->blend != bound_blend) {
      vkCmdBindPipeline(current_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[run->blend]);
      bound_blend = run->blend;
    }
    VkDescriptorSet set = pe_ui_image_set(run->image);
    vkCmdBindDescriptorSets(current_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,
                            &set, 0, NULL);
    vkCmdDraw(current_cmd, run->count, 1, run->first, 0);
  }
}
