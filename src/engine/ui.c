#include "ui.h"

#include <stdlib.h>
#include <string.h>

#include "file_loader.h"
#include "files.h"
#include "images.h"
#include "utils.h"

#include "renderer/shader_module.h"
#include "renderer/vk_buffer.h"
#include "renderer/vk_images.h"
#include "renderer/vulkan.h"

#define UI_MAX_IMAGES 512
#define UI_MAX_VERTICES (6 * 4096)
#define UI_MAX_RUNS 1024
#define UI_BLEND_COUNT 2

typedef struct UiVertex {
  float x, y;
  float u, v;
  float r, g, b, a;
} UiVertex;

struct PUiImage {
  char path[256];
  PTexture texture;
  VkDescriptorSet set;
};

typedef struct UiRun {
  PUiImage *image;
  PUiBlend blend;
  uint32_t first;
  uint32_t count;
} UiRun;

typedef struct UiFrame {
  PBuffer buffer;
  UiVertex *mapped;
} UiFrame;

static VkDescriptorSetLayout set_layout;
static VkPipelineLayout pipeline_layout;
static VkPipeline pipelines[UI_BLEND_COUNT];
static VkSampler sampler;
static VkDescriptorPool pool;

static PUiImage *images[UI_MAX_IMAGES];
static uint32_t image_count;
static PUiImage *white;

static UiFrame *frames;
static uint32_t frame_count;
static UiRun runs[UI_MAX_RUNS];
static uint32_t run_count;

static VkCommandBuffer current_cmd;
static uint32_t current_frame;
static uint32_t vertex_count;
static bool batching;
static bool ready;
static float screen_width = 1.f;
static float screen_height = 1.f;

static bool create_layout_and_sampler() {
  VkDescriptorSetLayoutBinding binding = {
      .binding = 0,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT};

  VkDescriptorSetLayoutCreateInfo layout_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = 1,
      .pBindings = &binding};

  VkPushConstantRange push_constant = {.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                                       .size = sizeof(float) * 2};

  VkPipelineLayoutCreateInfo pipeline_layout_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 1,
      .pSetLayouts = &set_layout,
      .pushConstantRangeCount = 1,
      .pPushConstantRanges = &push_constant};

  VkSamplerCreateInfo sampler_info = {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .magFilter = VK_FILTER_LINEAR,
      .minFilter = VK_FILTER_LINEAR,
      .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .maxLod = 1.f};

  return vkCreateDescriptorSetLayout(vk_device, &layout_info, NULL,
                                     &set_layout) == VK_SUCCESS &&
         vkCreatePipelineLayout(vk_device, &pipeline_layout_info, NULL,
                                &pipeline_layout) == VK_SUCCESS &&
         vkCreateSampler(vk_device, &sampler_info, NULL, &sampler) ==
             VK_SUCCESS;
}

static bool create_pool() {
  VkDescriptorPoolSize size = {
      .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = UI_MAX_IMAGES};

  VkDescriptorPoolCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .poolSizeCount = 1,
      .pPoolSizes = &size,
      .maxSets = UI_MAX_IMAGES};

  return vkCreateDescriptorPool(vk_device, &info, NULL, &pool) == VK_SUCCESS;
}

static VkPipelineColorBlendAttachmentState blend_state(PUiBlend blend) {
  VkPipelineColorBlendAttachmentState state = {
      .blendEnable = VK_TRUE,
      .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
      .dstColorBlendFactor = blend == PE_UI_BLEND_ADD
                                 ? VK_BLEND_FACTOR_ONE
                                 : VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      .colorBlendOp = VK_BLEND_OP_ADD,
      .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
      .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      .alphaBlendOp = VK_BLEND_OP_ADD,
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};
  return state;
}

static bool create_pipelines() {
  File vertex_file, fragment_file;
  if (load_file(file_ui_vert_spv, &vertex_file) == -1 ||
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

  VkVertexInputBindingDescription binding = {
      .stride = sizeof(UiVertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};

  VkVertexInputAttributeDescription attributes[3] = {
      {.location = 0,
       .format = VK_FORMAT_R32G32_SFLOAT,
       .offset = offsetof(UiVertex, x)},
      {.location = 1,
       .format = VK_FORMAT_R32G32_SFLOAT,
       .offset = offsetof(UiVertex, u)},
      {.location = 2,
       .format = VK_FORMAT_R32G32B32A32_SFLOAT,
       .offset = offsetof(UiVertex, r)}};

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
      .lineWidth = 1.f};

  VkPipelineMultisampleStateCreateInfo multisample = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = pe_vk_msaa_samples};

  VkPipelineDepthStencilStateCreateInfo depth = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};

  VkDynamicState dynamic_states[2] = {VK_DYNAMIC_STATE_VIEWPORT,
                                      VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamic = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .dynamicStateCount = 2,
      .pDynamicStates = dynamic_states};

  bool ok = vertex && fragment;
  for (int blend = 0; ok && blend < UI_BLEND_COUNT; blend++) {
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

    ok = vkCreateGraphicsPipelines(vk_device, VK_NULL_HANDLE, 1, &info, NULL,
                                   &pipelines[blend]) == VK_SUCCESS;
  }

  if (vertex)
    vkDestroyShaderModule(vk_device, vertex, NULL);
  if (fragment)
    vkDestroyShaderModule(vk_device, fragment, NULL);
  return ok;
}

static bool create_frames(uint32_t count) {
  frames = calloc(count, sizeof(UiFrame));
  if (!frames)
    return false;
  frame_count = count;

  size_t size = sizeof(UiVertex) * UI_MAX_VERTICES;
  void *zeros = calloc(1, size);
  for (uint32_t i = 0; i < count; i++) {
    frames[i].buffer =
        pe_vk_create_buffer(size, zeros, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    vkMapMemory(vk_device, frames[i].buffer.memory, 0, size, 0,
                (void **)&frames[i].mapped);
  }
  free(zeros);
  return true;
}

static PUiImage *image_from_pixels(const char *path, PImage *pixels) {
  if (image_count == UI_MAX_IMAGES) {
    LOG("ui: no room for image %s\n", path);
    return NULL;
  }

  PUiImage *image = calloc(1, sizeof(PUiImage));
  snprintf(image->path, sizeof(image->path), "%s", path);
  image->texture.width = pixels->width;
  image->texture.heigth = pixels->heigth;
  pe_vk_create_texture_from_image_format(&image->texture, pixels,
                                         VK_FORMAT_R8G8B8A8_SRGB, false);

  VkDescriptorSetAllocateInfo alloc = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = pool,
      .descriptorSetCount = 1,
      .pSetLayouts = &set_layout};
  vkAllocateDescriptorSets(vk_device, &alloc, &image->set);

  VkDescriptorImageInfo image_info = {
      .sampler = sampler,
      .imageView = image->texture.image_view,
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
  VkWriteDescriptorSet write = {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = image->set,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .pImageInfo = &image_info};
  vkUpdateDescriptorSets(vk_device, 1, &write, 0, NULL);

  images[image_count++] = image;
  return image;
}

int pe_ui_init() {
  if (!create_layout_and_sampler() || !create_pool() || !create_pipelines() ||
      !create_frames(main_render_target.images_count))
    return 0;

  unsigned char texel[4] = {255, 255, 255, 255};
  PImage pixels = {.width = 1, .heigth = 1, .pixels_data = texel};
  white = image_from_pixels("white", &pixels);

  ready = white != NULL;
  return ready;
}

void pe_ui_shutdown() {
  if (!ready)
    return;

  vkDeviceWaitIdle(vk_device);

  for (uint32_t i = 0; i < image_count; i++) {
    pe_vk_clean_image(&images[i]->texture);
    free(images[i]);
  }
  image_count = 0;

  for (uint32_t i = 0; i < frame_count; i++) {
    vkUnmapMemory(vk_device, frames[i].buffer.memory);
    pe_vk_destroy_buffer(&frames[i].buffer);
  }
  free(frames);
  frames = NULL;

  for (int blend = 0; blend < UI_BLEND_COUNT; blend++)
    vkDestroyPipeline(vk_device, pipelines[blend], NULL);
  vkDestroyPipelineLayout(vk_device, pipeline_layout, NULL);
  vkDestroyDescriptorPool(vk_device, pool, NULL);
  vkDestroyDescriptorSetLayout(vk_device, set_layout, NULL);
  vkDestroySampler(vk_device, sampler, NULL);

  ready = false;
}

PUiImage *pe_ui_image(const char *path) {
  for (uint32_t i = 0; i < image_count; i++)
    if (strcmp(images[i]->path, path) == 0)
      return images[i];

  PImage pixels;
  if (pe_load_image(path, &pixels) == -1)
    return NULL;

  PUiImage *image = image_from_pixels(path, &pixels);
  free_image(&pixels);
  return image;
}

PUiImage *pe_ui_image_white() { return white; }

void pe_ui_image_size(const PUiImage *image, float *width, float *height) {
  *width = image->texture.width;
  *height = image->texture.heigth;
}

void pe_ui_begin(VkCommandBuffer cmd, PRenderTarget *target,
                 uint32_t image_index) {
  if (!ready)
    return;

  current_cmd = cmd;
  current_frame = image_index;
  screen_width = target->extent.width;
  screen_height = target->extent.height;
  vertex_count = 0;
  run_count = 0;
  batching = true;
}

static void push_vertex(float x, float y, float u, float v,
                        const vec4 color) {
  UiVertex *vertex = &frames[current_frame].mapped[vertex_count++];
  *vertex = (UiVertex){x, y, u, v, color[0], color[1], color[2], color[3]};
}

static void extend_runs(const PUiQuad *quad, PUiImage *image) {
  UiRun *last = run_count ? &runs[run_count - 1] : NULL;
  if (last && last->image == image && last->blend == quad->blend) {
    last->count += 6;
    return;
  }
  if (run_count == UI_MAX_RUNS) {
    vertex_count -= 6;
    return;
  }
  runs[run_count++] = (UiRun){image, quad->blend, vertex_count - 6, 6};
}

void pe_ui_quad(const PUiQuad *quad) {
  if (!ready || !batching || vertex_count + 6 > UI_MAX_VERTICES)
    return;

  float x0 = quad->x, y0 = quad->y;
  float x1 = quad->x + quad->width, y1 = quad->y + quad->height;

  push_vertex(x0, y0, quad->u0, quad->v0, quad->color);
  push_vertex(x1, y0, quad->u1, quad->v0, quad->color);
  push_vertex(x1, y1, quad->u1, quad->v1, quad->color);
  push_vertex(x0, y0, quad->u0, quad->v0, quad->color);
  push_vertex(x1, y1, quad->u1, quad->v1, quad->color);
  push_vertex(x0, y1, quad->u0, quad->v1, quad->color);

  extend_runs(quad, quad->image ? quad->image : white);
}

void pe_ui_end() {
  if (!ready || !batching)
    return;
  batching = false;

  if (run_count == 0)
    return;

  VkViewport viewport = {.width = screen_width,
                         .height = screen_height,
                         .maxDepth = 1.f};
  VkRect2D scissor = {.extent = {(uint32_t)screen_width,
                                 (uint32_t)screen_height}};
  float push_constants[2] = {screen_width, screen_height};
  VkDeviceSize offset = 0;

  vkCmdSetViewport(current_cmd, 0, 1, &viewport);
  vkCmdSetScissor(current_cmd, 0, 1, &scissor);
  vkCmdPushConstants(current_cmd, pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT,
                     0, sizeof(push_constants), push_constants);
  vkCmdBindVertexBuffers(current_cmd, 0, 1, &frames[current_frame].buffer.buffer,
                         &offset);

  int bound_blend = -1;
  for (uint32_t i = 0; i < run_count; i++) {
    UiRun *run = &runs[i];

    if ((int)run->blend != bound_blend) {
      vkCmdBindPipeline(current_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                        pipelines[run->blend]);
      bound_blend = run->blend;
    }
    vkCmdBindDescriptorSets(current_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipeline_layout, 0, 1, &run->image->set, 0, NULL);
    vkCmdDraw(current_cmd, run->count, 1, run->first, 0);
  }
}
