
#include "vk_vertex.h"
#include "vk_buffer.h"
#include "vk_memory.h"
#include "vulkan.h"
#include <engine/array.h>
#include <engine/macros.h>
#include <engine/vertex.h>

#include <engine/model.h>

#include "descriptor_set.h"
#include "uniform_buffer.h"

VkBuffer vertex_buffer;
VkBuffer index_buffer;

VkVertexInputBindingDescription pe_vk_vertex_get_binding_description() {
  VkVertexInputBindingDescription binding = {.binding = 0,
                                             .stride = sizeof(PVertex),
                                             .inputRate =
                                                 VK_VERTEX_INPUT_RATE_VERTEX};
  return binding;
}

void pe_vk_vertex_get_attribute(PVertexAtrributes *attributes) {

  if (attributes->position) {
    VkVertexInputAttributeDescription attribute;
    ZERO(attribute);

    attribute.binding = 0;
    attribute.location = 0;

    attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute.offset = offsetof(PVertex, position);
    array_add(&attributes->attributes_descriptions, &attribute);
  }
  if (attributes->color) {
    VkVertexInputAttributeDescription attribute;
    ZERO(attribute);

    attribute.binding = 0;
    attribute.location = 1;

    attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute.offset = offsetof(PVertex, color);
    array_add(&attributes->attributes_descriptions, &attribute);
  }
  if (attributes->normal) {
    VkVertexInputAttributeDescription attribute;
    ZERO(attribute);

    attribute.binding = 0;
    attribute.location = 2;

    attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute.offset = offsetof(PVertex, normal);
    array_add(&attributes->attributes_descriptions, &attribute);
  }
  if (attributes->uv) {
    VkVertexInputAttributeDescription attribute;
    ZERO(attribute);

    attribute.binding = 0;
    attribute.location = 3;//TODO maybe could be 3

    attribute.format = VK_FORMAT_R32G32_SFLOAT;
    attribute.offset = offsetof(PVertex, uv);
    array_add(&attributes->attributes_descriptions, &attribute);
  }
  if (attributes->joint) {
    VkVertexInputAttributeDescription attribute;
    ZERO(attribute);

    attribute.binding = 0;
    attribute.location = 4;

    attribute.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attribute.offset = offsetof(PVertex, joint);
    array_add(&attributes->attributes_descriptions, &attribute);
  }
  if (attributes->weight) {
    VkVertexInputAttributeDescription attribute;
    ZERO(attribute);

    attribute.binding = 0;
    attribute.location = 5;

    attribute.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attribute.offset = offsetof(PVertex, weight);
    array_add(&attributes->attributes_descriptions, &attribute);
  }
}

//these have to outlive the pe_vk_vertex_get_instanced_input() call because
//the pipeline create info only keeps pointers to them
static VkVertexInputBindingDescription instanced_bindings[2];
static VkVertexInputAttributeDescription instanced_attributes[9];

VkPipelineVertexInputStateCreateInfo pe_vk_vertex_get_instanced_input() {

  ZERO(instanced_bindings);
  ZERO(instanced_attributes);

  //the mesh, stepped once per vertex
  instanced_bindings[0].binding = 0;
  instanced_bindings[0].stride = sizeof(PVertex);
  instanced_bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  //the instances, stepped once per copy of the mesh
  instanced_bindings[1].binding = 1;
  instanced_bindings[1].stride = sizeof(PInstance);
  instanced_bindings[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

  //locations 0-5 belong to PVertex, so instance data starts at 6
  instanced_attributes[0].binding = 0;
  instanced_attributes[0].location = 0;
  instanced_attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  instanced_attributes[0].offset = offsetof(PVertex, position);

  instanced_attributes[1].binding = 0;
  instanced_attributes[1].location = 2;
  instanced_attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  instanced_attributes[1].offset = offsetof(PVertex, normal);

  instanced_attributes[2].binding = 0;
  instanced_attributes[2].location = 3;
  instanced_attributes[2].format = VK_FORMAT_R32G32_SFLOAT;
  instanced_attributes[2].offset = offsetof(PVertex, uv);

  instanced_attributes[3].binding = 1;
  instanced_attributes[3].location = 6;
  instanced_attributes[3].format = VK_FORMAT_R32G32B32_SFLOAT;
  instanced_attributes[3].offset = offsetof(PInstance, position);

  instanced_attributes[4].binding = 1;
  instanced_attributes[4].location = 7;
  instanced_attributes[4].format = VK_FORMAT_R32G32B32_SFLOAT;
  instanced_attributes[4].offset = offsetof(PInstance, scale);

  instanced_attributes[5].binding = 1;
  instanced_attributes[5].location = 8;
  instanced_attributes[5].format = VK_FORMAT_R32G32B32_SFLOAT;
  instanced_attributes[5].offset = offsetof(PInstance, color);

  //the name, 4 characters packed per uint, split over two uvec4
  instanced_attributes[6].binding = 1;
  instanced_attributes[6].location = 9;
  instanced_attributes[6].format = VK_FORMAT_R32G32B32A32_UINT;
  instanced_attributes[6].offset = offsetof(PInstance, name);

  instanced_attributes[7].binding = 1;
  instanced_attributes[7].location = 10;
  instanced_attributes[7].format = VK_FORMAT_R32G32B32A32_UINT;
  instanced_attributes[7].offset = offsetof(PInstance, name) + sizeof(u32) * 4;

  instanced_attributes[8].binding = 1;
  instanced_attributes[8].location = 11;
  instanced_attributes[8].format = VK_FORMAT_R32_SFLOAT;
  instanced_attributes[8].offset = offsetof(PInstance, name_length);

  VkPipelineVertexInputStateCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount = 2,
      .pVertexBindingDescriptions = instanced_bindings,
      .vertexAttributeDescriptionCount = 9,
      .pVertexAttributeDescriptions = instanced_attributes};

  return info;
}



