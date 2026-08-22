#include "uniform_buffer.h"
#include "cglm/affine.h"
#include "cglm/types.h"
#include "cglm/vec4.h"

#include "engine/model.h"
#include <cglm/mat4.h>
#include "vk_buffer.h"

#include <stdint.h>
#include <vulkan/vulkan_core.h>
#include "swap_chain.h"
#include "vulkan.h"


PUniformBufferObject ubo;

PBufferCreateInfo buffer_color;

static PUniformBufferObject cube_ubo;

void pe_vk_ubo_init() {
  ZERO(ubo);
  glm_mat4_identity(ubo.projection);
  glm_mat4_identity(ubo.view);
  glm_mat4_identity(ubo.model);
}

PBufferCreateInfo pe_vk_uniform_buffer_create_buffer(size_t size) {

  PBufferCreateInfo info;
  ZERO(info);
  info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  info.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

  info.size = size;

  pe_vk_create_buffer_memory(&info);
  return info;
}

void pe_vk_create_uniform_buffers(PModel *model, PRenderTarget *target) {
  VkDeviceSize buffer_size = sizeof(PUniformBufferObject);

  //INFO a model is drawn on every render target, indexed each time by that
  //target's own image_index, but it owns only one uniform_buffers array - so
  //it has to be sized for whichever target has the most swap chain images,
  //not just the one passed in here
  u32 count = pe_vk_targets_max_images_count();

  array_init(&model->uniform_buffers, sizeof(VkBuffer), count);

  array_init(&model->uniform_buffers_memory, sizeof(VkDeviceMemory), count);

  //LOG("Creating uniform buffer\n");
  for (int i = 0; i < count; i++) {
    // create buffer
    PBufferCreateInfo info =
        pe_vk_uniform_buffer_create_buffer(sizeof(PUniformBufferObject));

    array_add(&model->uniform_buffers, &info.buffer);
    array_add(&model->uniform_buffers_memory, &info.buffer_memory);
  }

  // buffer_color = pe_vk_uniform_buffer_create_buffer(sizeof(PEColorShader));
}

void pe_vk_memory_copy(size_t size, VkDeviceMemory *memory, void *in_data) {

  void *data;
  vkMapMemory(vk_device, *(memory), 0, size, 0, &data);
  memcpy(data, in_data, size);
  vkUnmapMemory(vk_device, *(memory));
}

void pe_vk_send_uniform_buffer(PModel* model, uint32_t image_index){

  PUniformBufferObject buffers[] = {model->uniform_buffer_object};

  VkDeviceMemory *memory =
      array_get(&model->uniform_buffers_memory, image_index);

  pe_vk_memory_copy(sizeof(buffers), memory, buffers);
}


