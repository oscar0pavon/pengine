#include "terrain_pipeline.h"
#include "terrain_building.h"
#include "terrain_mesh.h"
#include "terrain_water.h"

#include <engine/files.h>
#include <engine/macros.h>
#include <engine/renderer/pipeline.h>
#include <engine/renderer/vulkan.h>

#include <stddef.h>

//INFO the mesh winds counter clockwise seen from above, which the projection's
//Y flip turns clockwise on the screen. with counter clockwise here instead, the
//ground facing the camera is the side that gets culled
#define TERRAIN_CULL_MODE VK_CULL_MODE_BACK_BIT
#define TERRAIN_FRONT_FACE VK_FRONT_FACE_CLOCKWISE


static VkDescriptorSetLayout create_set_layout(
    const VkDescriptorSetLayoutBinding *bindings, u32 count) {
  VkDescriptorSetLayoutCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = count,
      .pBindings = bindings};

  VkDescriptorSetLayout layout;
  VKVALID(vkCreateDescriptorSetLayout(vk_device, &info, NULL, &layout),
          "Can't create terrain descriptor set layout");
  return layout;
}

static void create_layouts(PTerrainPipeline *pipeline) {
  VkDescriptorSetLayoutBinding frame = {
      .binding = 0,
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT};
  pipeline->frame_layout = create_set_layout(&frame, 1);

  VkDescriptorSetLayoutBinding material[] = {
      {.binding = 0,
       .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
       .descriptorCount = PE_TERRAIN_LAYERS_MAX,
       .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT},
      {.binding = 1,
       .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
       .descriptorCount = 1,
       .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT}};
  pipeline->material_layout = create_set_layout(material, 2);

  VkDescriptorSetLayout set_layouts[] = {pipeline->frame_layout,
                                         pipeline->material_layout};
  VkPipelineLayoutCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 2,
      .pSetLayouts = set_layouts};
  VKVALID(vkCreatePipelineLayout(vk_device, &info, NULL, &pipeline->layout),
          "Can't create terrain pipeline layout");
}

//the sky reads no vertex buffer, and neither tests nor writes depth: it is
//drawn first and everything else goes over it
static void create_sky(PTerrainPipeline *pipeline) {
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
  info.out_shader = &pipeline->sky;
  info.vertex_path = file_sky_vert_spv;
  info.fragment_path = file_sky_frag_spv;
  info.layout = pipeline->layout;
  info.vertex_input = &no_input;
  info.rasterization = &rasterization;
  info.depth_stencil = &depth_stencil;

  pe_vk_create_shader(&info);
}

//INFO water is blended over the ground, so it reads the depth the ground left
//but does not write its own: two surfaces of it that overlap, a river running
//into a lake, would otherwise hide one another
static void create_water(PTerrainPipeline *pipeline) {
  VkVertexInputBindingDescription binding = {
      .binding = 0,
      .stride = sizeof(PTerrainWaterVertex),
      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};

  VkVertexInputAttributeDescription attributes[] = {
      {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(PTerrainWaterVertex, position)},
      {1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(PTerrainWaterVertex, depth)}};

  VkPipelineVertexInputStateCreateInfo vertex_input = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount = 1,
      .pVertexBindingDescriptions = &binding,
      .vertexAttributeDescriptionCount = 2,
      .pVertexAttributeDescriptions = attributes};

  //seen from above and from below both
  VkPipelineRasterizationStateCreateInfo rasterization = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = VK_CULL_MODE_NONE,
      .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
      .lineWidth = 1.0f};

  VkPipelineDepthStencilStateCreateInfo depth_stencil = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .depthTestEnable = VK_TRUE,
      .depthWriteEnable = VK_FALSE,
      .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
      .stencilTestEnable = VK_FALSE};

  VkPipelineColorBlendAttachmentState attachment = {
      .blendEnable = VK_TRUE,
      .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
      .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      .colorBlendOp = VK_BLEND_OP_ADD,
      .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
      .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
      .alphaBlendOp = VK_BLEND_OP_ADD,
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};

  VkPipelineColorBlendStateCreateInfo color_blend = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .attachmentCount = 1,
      .pAttachments = &attachment};

  PCreateShaderInfo info;
  ZERO(info);
  info.out_shader = &pipeline->water;
  info.vertex_path = file_water_vert_spv;
  info.fragment_path = file_water_frag_spv;
  info.layout = pipeline->layout;
  info.vertex_input = &vertex_input;
  info.rasterization = &rasterization;
  info.depth_stencil = &depth_stencil;
  info.color_blend = &color_blend;

  pe_vk_create_shader(&info);
}

//INFO the model matrix is 64 bytes for the vertex stage and the alpha cutoff
//is 4 more for the fragment stage, at 64: both fit in the 128 bytes every
//device has, and neither needs a descriptor set of its own, which would have to
//be rewritten for every placement of every building
static void create_building_layout(PTerrainPipeline *pipeline) {
  VkDescriptorSetLayoutBinding texture = {
      .binding = 0,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT};
  pipeline->building_material_layout = create_set_layout(&texture, 1);

  VkPushConstantRange ranges[] = {
      {.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
       .offset = 0,
       .size = sizeof(mat4)},
      {.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
       .offset = sizeof(mat4),
       .size = sizeof(float)}};

  VkDescriptorSetLayout set_layouts[] = {pipeline->frame_layout,
                                         pipeline->building_material_layout};
  VkPipelineLayoutCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 2,
      .pSetLayouts = set_layouts,
      .pushConstantRangeCount = 2,
      .pPushConstantRanges = ranges};
  VKVALID(vkCreatePipelineLayout(vk_device, &info, NULL,
                                 &pipeline->building_layout),
          "Can't create building pipeline layout");
}

//nothing is culled: a wall is a single sheet with two sides, and the
//placement's reflection turns the winding of every triangle inside out
static void create_building(PTerrainPipeline *pipeline) {
  create_building_layout(pipeline);

  VkVertexInputBindingDescription binding = {
      .binding = 0,
      .stride = sizeof(PBuildingVertex),
      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};

  VkVertexInputAttributeDescription attributes[] = {
      {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(PBuildingVertex, position)},
      {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(PBuildingVertex, normal)},
      {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(PBuildingVertex, uv)},
      {3, 0, VK_FORMAT_R8G8B8A8_UNORM, offsetof(PBuildingVertex, color)}};

  VkPipelineVertexInputStateCreateInfo vertex_input = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount = 1,
      .pVertexBindingDescriptions = &binding,
      .vertexAttributeDescriptionCount = 4,
      .pVertexAttributeDescriptions = attributes};

  VkPipelineRasterizationStateCreateInfo rasterization = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = VK_CULL_MODE_NONE,
      .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
      .lineWidth = 1.0f};

  PCreateShaderInfo info;
  ZERO(info);
  info.out_shader = &pipeline->building;
  info.vertex_path = file_building_vert_spv;
  info.fragment_path = file_building_frag_spv;
  info.layout = pipeline->building_layout;
  info.vertex_input = &vertex_input;
  info.rasterization = &rasterization;

  pe_vk_create_shader(&info);
}

void pe_vk_terrain_pipeline_create(PTerrainPipeline *pipeline) {
  create_layouts(pipeline);

  VkVertexInputBindingDescription binding = {
      .binding = 0,
      .stride = sizeof(PTerrainVertex),
      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};

  VkVertexInputAttributeDescription attributes[] = {
      {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(PTerrainVertex, position)},
      {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(PTerrainVertex, normal)},
      {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(PTerrainVertex, uv)},
      {3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(PTerrainVertex, layer_uv)}};

  VkPipelineVertexInputStateCreateInfo vertex_input = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount = 1,
      .pVertexBindingDescriptions = &binding,
      .vertexAttributeDescriptionCount = 4,
      .pVertexAttributeDescriptions = attributes};

  VkPipelineRasterizationStateCreateInfo rasterization = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = TERRAIN_CULL_MODE,
      .frontFace = TERRAIN_FRONT_FACE,
      .lineWidth = 1.0f};

  PCreateShaderInfo info;
  ZERO(info);
  info.out_shader = &pipeline->shader;
  info.vertex_path = file_terrain_vert_spv;
  info.fragment_path = file_terrain_frag_spv;
  info.layout = pipeline->layout;
  info.vertex_input = &vertex_input;
  info.rasterization = &rasterization;

  pe_vk_create_shader(&info);

  create_sky(pipeline);
  create_water(pipeline);
  create_building(pipeline);
}

static VkDescriptorPool create_frames_pool(u32 count) {
  VkDescriptorPoolSize size = {.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                               .descriptorCount = count};
  VkDescriptorPoolCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = count,
      .poolSizeCount = 1,
      .pPoolSizes = &size};

  VkDescriptorPool pool;
  VKVALID(vkCreateDescriptorPool(vk_device, &info, NULL, &pool),
          "Can't create terrain frame descriptor pool");
  return pool;
}

void pe_vk_terrain_frames_create(const PTerrainPipeline *pipeline,
                                 PTerrainFrames *frames) {
  frames->count = pe_vk_targets_max_images_count();
  frames->pool = create_frames_pool(frames->count);

  PTerrainFrame empty;
  ZERO(empty);

  VkDescriptorSetLayout layouts[PE_VK_MAX_SWAPCHAIN_IMAGES];
  for (u32 i = 0; i < frames->count; i++) {
    layouts[i] = pipeline->frame_layout;
    frames->buffers[i] = pe_vk_create_buffer(
        sizeof(empty), &empty, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
  }

  VkDescriptorSetAllocateInfo allocation = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = frames->pool,
      .descriptorSetCount = frames->count,
      .pSetLayouts = layouts};
  VKVALID(vkAllocateDescriptorSets(vk_device, &allocation, frames->sets),
          "Can't allocate terrain frame descriptor sets");

  for (u32 i = 0; i < frames->count; i++) {
    VkDescriptorBufferInfo buffer = {.buffer = frames->buffers[i].buffer,
                                     .range = sizeof(PTerrainFrame)};
    VkWriteDescriptorSet write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = frames->sets[i],
        .dstBinding = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &buffer};
    vkUpdateDescriptorSets(vk_device, 1, &write, 0, NULL);
  }
}

void pe_vk_terrain_frame_update(PTerrainFrames *frames, u32 image_index,
                                const PTerrainFrame *frame) {
  pe_vk_update_buffer(&frames->buffers[image_index], (void *)frame,
                      sizeof(*frame));
}
