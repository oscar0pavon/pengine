#include <engine/renderer/vulkan/vulkan.h>


VkPipeline pe_vk_pipeline;

Array pe_vk_pipeline_infos;
Array pe_graphics_pipelines;

#define PE_VK_PIPELINES_MAX 50

void pe_vk_pipeline_init();
void pe_vk_pipeline_create_layout();
