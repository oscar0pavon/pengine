#ifndef PE_VK_SHADER_MODULE_H
#define PE_VK_SHADER_MODULE_H

#include "vulkan.h"
#include "shaders.h"

#include <engine/file_loader.h>

void pe_vk_shader_load(PCreateShaderInfo *info);
VkShaderModule pe_vk_shader_module_create(File *file);

#endif
