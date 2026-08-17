#include <engine/renderer/vulkan/vulkan.h>

 VkPipelineShaderStageCreateInfo pe_vk_shaders_stages_infos[2];

void pe_vk_shader_load(VkPipelineShaderStageCreateInfo* infos, const char *vert_path, const char* frag_path);
