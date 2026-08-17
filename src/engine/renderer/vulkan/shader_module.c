#include <engine/renderer/vulkan/vulkan.h>
#include <engine/file_loader.h>
#include "shader_module.h"
#include <engine/log.h>

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

void pe_vk_shader_load(VkPipelineShaderStageCreateInfo* infos, const char *vert_path, const char* frag_path) {
  File new_file;
  ZERO(new_file);
  pe_file_openb(vert_path, &new_file);

  File new_file2;
  ZERO(new_file2);
  pe_file_openb(frag_path,  &new_file2);

  VkShaderModule module1 = pe_vk_shader_module_create(&new_file);
  VkShaderModule module2 = pe_vk_shader_module_create(&new_file2);

  VkPipelineShaderStageCreateInfo info1;
  ZERO(info1);
  info1.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  info1.stage = VK_SHADER_STAGE_VERTEX_BIT;
  info1.module = module1;
  info1.pName = "main";

  VkPipelineShaderStageCreateInfo info2;
  ZERO(info2);
  info2.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  info2.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  info2.module = module2;
  info2.pName = "main";

  infos[0] = info1;
  infos[1] = info2;
}
