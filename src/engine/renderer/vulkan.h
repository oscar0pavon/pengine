#ifndef PEVULKAN_H
#define PEVULKAN_H

#include <vulkan/vulkan_core.h>
#define VK_USE_PLATFORM_WAYLAND_KHR // Must be defined before including vulkan.h
#include <vulkan/vulkan.h>

#include <engine/array.h>
#include <engine/macros.h>

#include <engine/log.h>

#include <cglm/cglm.h>
#include "render_target.h"

#define VEC3(p1, p2, p3)                                                       \
  (vec3) { p1, p2, p3 }
#define VEC4(p1, p2, p3, p4)                                                   \
  (vec4) { p1, p2, p3, p4 }

#define VEC2(p1, p2)                                                           \
  (vec2) { p1, p2 }

#define VKVALID(f,message) if(f != VK_SUCCESS){LOG("%s \n",message);}

typedef struct PUniformBufferObject {
  mat4 model;
  mat4 view;
  mat4 projection;
  vec4 light_position;
  //INFO a shader that does not declare this member simply does not read it,
  //so the trailing field costs the others nothing
  vec4 color;
} PUniformBufferObject;

extern PFN_vkGetMemoryFdKHR pe_vk_get_memory_file_descriptor;


int pe_vk_init();
void pe_vk_end();

  
extern VkPhysicalDevice vk_physical_device;

extern PRenderTarget pe_render_targets[PE_VK_MAX_RENDER_TARGETS];
extern u32 pe_render_targets_count;

//INFO a model's uniform buffers/descriptor sets are shared across every
//target it is drawn on (see uniform_buffer.c, descriptor_set.c), so they have
//to be sized for whichever target has the most swap chain images, not just
//targets[0]
u32 pe_vk_targets_max_images_count(void);

//most of the tree still names the one render target that existed before
//multimonitor; this keeps every one of those call sites compiling unchanged
#define main_render_target (pe_render_targets[0])

extern VkInstance vk_instance;
extern VkDevice vk_device;
extern VkQueue vk_queue;

extern VkRenderPass pe_vk_render_pass;

extern VkSampleCountFlagBits pe_vk_msaa_samples;

extern uint32_t q_graphic_family;
extern uint32_t q_present_family;


extern bool pe_vk_validation_layer_enable;

extern bool pe_vk_initialized;

extern Array buffers;

#endif
