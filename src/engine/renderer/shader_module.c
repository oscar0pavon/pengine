#include "shader_module.h"
#include "shaders.h"
#include "vulkan.h"
#include <engine/log.h>
#include "pipeline.h"


VkShaderModule pe_vk_shader_module_create(File *file) {
  VkShaderModuleCreateInfo info;
  ZERO(info);
  info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  info.codeSize = file->size_in_bytes;
  info.pCode = file->data;

  VkShaderModule module;
  VKVALID(vkCreateShaderModule(vk_device, &info, NULL, &module),
          "Can't create Shader Module");
  return module;
}

