#include "terrain_pipeline.h"
#include "terrain_mesh.h"

#include <engine/files.h>
#include <engine/macros.h>
#include <engine/renderer/pipeline.h>
#include <engine/renderer/vulkan.h>

#include <stddef.h>


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

  PCreateShaderInfo info;
  ZERO(info);
  info.out_shader = &pipeline->shader;
  info.vertex_path = file_terrain_vert_spv;
  info.fragment_path = file_terrain_frag_spv;
  info.layout = pipeline->layout;
  info.vertex_input = &vertex_input;

  pe_vk_create_shader(&info);
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
