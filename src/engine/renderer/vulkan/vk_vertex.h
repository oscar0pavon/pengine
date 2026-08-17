#include <engine/renderer/vulkan/vulkan.h>
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

VkBuffer pe_vk_vertex_create_buffer(Array* vertices);

VkBuffer pe_vk_vertex_create_index_buffer(Array* indices);

void pe_vk_models_create();

VkBuffer vertex_buffer;
VkBuffer index_buffer;


