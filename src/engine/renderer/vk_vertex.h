#ifndef VK_VERTEX_H
#define VK_VERTEX_H

#include "vulkan.h"
#include <engine/array.h>

typedef struct PVertexAtrributes {
  bool has_attributes;
  bool position;
  bool color;
  bool normal;
  bool uv;
  bool weight;
  bool joint;
  Array attributes_descriptions;
} PVertexAtrributes;

VkVertexInputBindingDescription pe_vk_vertex_get_binding_description();
void pe_vk_vertex_get_attribute(PVertexAtrributes* attributes);

//two bindings: binding 0 walks the mesh per vertex, binding 1 walks
//a PInstance array once per copy of that mesh
VkPipelineVertexInputStateCreateInfo pe_vk_vertex_get_instanced_input();


extern VkBuffer vertex_buffer;
extern VkBuffer index_buffer;

#endif

