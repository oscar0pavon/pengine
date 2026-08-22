#include "vulkan.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "commands.h"
#include "descriptor_set.h"
#include "engine/array.h"
#include "engine/images.h"
#include "framebuffer.h"
#include "images_view.h"
#include "pipeline.h"
#include "render_pass.h"
#include "shader_module.h"
#include "swap_chain.h"
#include "sync.h"
#include "uniform_buffer.h"
#include "vk_images.h"
#include "vk_vertex.h"
#include <engine/log.h>
#include <engine/macros.h>
#include <vulkan/vulkan_core.h>
#include <wchar.h>

#include "renderer.h"

#include "display.h"
#include "physical_devices.h"
#include "logical_device.h"
#include "instance.h"
#include "debug.h"
#include "queues.h"
#include "surface.h"

#include "render_target.h"

VkInstance vk_instance;
VkDevice vk_device;

bool is_drm_rendering = false;
bool is_wayland_window = false;

uint32_t pe_window_width = 1280;
uint32_t pe_window_height = 720;

PRenderTarget pe_render_targets[PE_VK_MAX_RENDER_TARGETS];
u32 pe_render_targets_count = 1;

VkRenderPass pe_vk_render_pass;

VkSampleCountFlagBits pe_vk_msaa_samples;


bool pe_vk_initialized;



PFN_vkGetMemoryFdKHR pe_vk_get_memory_file_descriptor;


Array buffers;

u32 pe_vk_targets_max_images_count(void) {
  u32 max = 0;
  for (u32 i = 0; i < pe_render_targets_count; i++)
    if (pe_render_targets[i].images_count > max)
      max = pe_render_targets[i].images_count;
  return max;
}

void pe_vk_end() {

  pe_vk_clean_commands();

  for (u32 t = 0; t < pe_render_targets_count; t++) {
    PRenderTarget *target = &pe_render_targets[t];

    vkDestroyCommandPool(vk_device, target->commands_pool, NULL);

    vkDestroySwapchainKHR(vk_device, target->swap_chain, NULL);

    pe_vk_end_sync(target);

    vkDestroyImageView(vk_device, target->depth_image_view, NULL);
    vkDestroyImage(vk_device, target->depth_image, NULL);
    vkFreeMemory(vk_device, target->depth_memory, NULL);

    vkDestroyImageView(vk_device, target->color_image_view, NULL);
    vkDestroyImage(vk_device, target->color_image, NULL);
    vkFreeMemory(vk_device, target->color_memory, NULL);

    for (int i = 0; i < target->images_views.count; i++) {
      VkFramebuffer *framebuffer = array_get(&target->framebuffers, i);
      vkDestroyFramebuffer(vk_device, *framebuffer, NULL);
    }

    for (int i = 0; i < target->images_views.count; i++) {
      VkImageView *image_view = array_get(&target->images_views, i);
      vkDestroyImageView(vk_device, *image_view, NULL);
    }

    vkDestroySurfaceKHR(vk_instance, target->surface, NULL);
  }

  pe_vk_clean_layouts();

  pe_vk_clean_descriptors_set();

  vkDestroyRenderPass(vk_device, pe_vk_render_pass, NULL);

  for(int i = 0; i < buffers.count; i++){
    VkBuffer* buffer = array_get(&buffers, i);
    vkDestroyBuffer(vk_device,*buffer,NULL);
  }
  pe_vk_debug_end();
  vkDestroyDevice(vk_device, NULL);
  vkDestroyInstance(vk_instance, NULL);
}

int pe_vk_init() {
  pe_vk_msaa_samples = VK_SAMPLE_COUNT_4_BIT;

  //INFO every buffer pe_vk_create_buffer_memory() makes is registered here so
  //pe_vk_end() can destroy it. that is one entry per model per swap chain image
  //for the uniform buffers alone, so this is a starting size and not a bound -
  //it was a bound before Array could grow, and chess ran into it at 512
  array_init(&buffers, sizeof(VkBuffer), 512);

  pe_vk_create_instance();

  pe_vk_get_physical_device();
  
  pe_vk_queue_families_support();

  pe_vk_create_logical_device();
 
  if(is_drm_rendering){
    pe_vk_get_memory_file_descriptor =
        (PFN_vkGetMemoryFdKHR)vkGetDeviceProcAddr(vk_device, "vkGetMemoryFdKHR");

    if(!pe_vk_get_memory_file_descriptor){
      printf("Error can't get vulkan extenstion\n");
      return 1;
    }
    vk_get_displays();
    pe_render_targets_count = pe_vk_displays_count;
  }

  vkGetDeviceQueue(vk_device, q_graphic_family, 0, &vk_queue);

  for (u32 t = 0; t < pe_render_targets_count; t++) {
    PRenderTarget *target = &pe_render_targets[t];
    pe_vk_create_surface(target, t);
    pe_vk_create_swapchain(target);
    pe_vk_set_viewport_and_sccisor(target);
    pe_vk_create_images_views(target);
  }

  //INFO the swap chain is the authority on the real render size - under DRM
  //it comes from the display mode, not the 1280x720 default - and everything
  //still reading the old globals (camera.c, engine2d.c) picks it up from here
  pe_window_width = pe_render_targets[0].width;
  pe_window_height = pe_render_targets[0].heigth;

  //INFO the render pass, layouts and pipelines are shared by every target
  //(render_pass.h documents why) so they only need targets[0]'s format
  for (u32 t = 1; t < pe_render_targets_count; t++) {
    if (pe_render_targets[t].format != pe_render_targets[0].format) {
      printf("ERROR display %i's surface format does not match display 0's - "
             "multimonitor needs a shared render pass. Exit\n", t);
      exit(1);
    }
  }

  pe_vk_create_render_pass(&pe_render_targets[0]);

  pe_vk_create_descriptor_set_layout();

  pe_vk_create_descriptor_set_layout_with_texture();
  //pe_vk_create_descriptor_set_layout_skinned();


  pe_vk_pipeline_create_layout(true, &pe_vk_pipeline_layout_with_descriptors,
                               &pe_vk_descriptor_set_layout);

  pe_vk_pipeline_create_layout(true, &pe_vk_pipeline_layout3,
                               &pe_vk_descriptor_set_layout_with_texture);

  // pe_vk_pipeline_create_layout(true, &pe_vk_pipeline_layout_skinned,
  //                              &pe_vk_descriptor_set_layout_skinned);

  pe_vk_pipelines_init(&pe_render_targets[0]);

  pe_vk_initialized = true;

  pe_vk_commands_pool_init();

  for (u32 t = 0; t < pe_render_targets_count; t++) {
    PRenderTarget *target = &pe_render_targets[t];
    pe_vk_target_command_pool_init(target);
    pe_vk_create_color_resources(target);
    pe_vk_create_depth_resources(target);
    pe_vk_framebuffer_create(target);
    pe_vk_command_init(target);
    pe_vk_semaphores_create(target);
    camera_init_with_size(&target->camera, target->width, target->heigth);
  }

  LOG("Vulkan intialize [OK]\n");
  return 0;
}
