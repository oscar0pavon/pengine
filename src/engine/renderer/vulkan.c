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

VkInstance vk_instance;
VkDevice vk_device;

bool is_drm_rendering = false;
bool is_wayland_window = false;

uint32_t pe_window_width = 1280;
uint32_t pe_window_height = 720;


VkRenderPass pe_vk_render_pass;

VkSampleCountFlagBits pe_vk_msaa_samples;

VkViewport viewport;
VkRect2D scissor;


bool pe_vk_initialized;



PFN_vkGetMemoryFdKHR pe_vk_get_memory_file_descriptor;


Array buffers;

void pe_vk_end() {

  pe_vk_clean_commands();

  vkDestroySwapchainKHR(vk_device, pe_vk_swap_chain, NULL);

  pe_vk_clean_layouts();

  pe_vk_end_sync();

  pe_vk_clean_descriptors_set();

  vkDestroyRenderPass(vk_device, pe_vk_render_pass, NULL);

  pe_vk_clean_image(&vk_depth_image);
  pe_vk_clean_image(&vk_color_image);

  vkDestroyImageView(vk_device, pe_vk_color_image_view, NULL);
  vkDestroyImageView(vk_device, pe_vk_depth_image_view, NULL);

  for(int i = 0; i < pe_vk_images_views.count; i++){
    VkFramebuffer *framebuffer = array_get(&pe_vk_framebuffers, i);
    vkDestroyFramebuffer(vk_device, *framebuffer, NULL);
  }

  for(int i = 0; i < pe_vk_images_views.count; i++){
    VkImageView *image_view = array_get(&pe_vk_images_views, i) ;
    vkDestroyImageView(vk_device, *image_view, NULL);
  }

  for(int i = 0; i < buffers.count; i++){
    VkBuffer* buffer = array_get(&buffers, i);
    vkDestroyBuffer(vk_device,*buffer,NULL);
  }
  pe_vk_debug_end();
  vkDestroySurfaceKHR(vk_instance, vk_surface, NULL);
  vkDestroyDevice(vk_device, NULL);
  vkDestroyInstance(vk_instance, NULL);
}

int pe_vk_init() {
  pe_vk_msaa_samples = VK_SAMPLE_COUNT_4_BIT;

  //INFO every buffer pe_vk_create_buffer_memory() makes is registered here so
  //pe_vk_end() can destroy it, and Array does not reallocate. that is one entry
  //per model per swap chain image for the uniform buffers alone, so the bound
  //is objects * images, not objects
  array_init(&buffers, sizeof(VkBuffer), 8192);

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
  }

  vkGetDeviceQueue(vk_device, q_graphic_family, 0, &vk_queue);

  pe_vk_create_surface();

  pe_vk_create_swapchain();

  if(is_drm_rendering){
    //pe_vk_create_exportable_images();
    //init_direct_render();
  }

  pe_vk_set_viewport_and_sccisor();

  pe_vk_create_images_views();

  pe_vk_create_render_pass();

  pe_vk_create_descriptor_set_layout();
  
  pe_vk_create_descriptor_set_layout_with_texture();
  //pe_vk_create_descriptor_set_layout_skinned();

  
  pe_vk_pipeline_create_layout(true, &pe_vk_pipeline_layout_with_descriptors,
                               &pe_vk_descriptor_set_layout);

  pe_vk_pipeline_create_layout(true, &pe_vk_pipeline_layout3,
                               &pe_vk_descriptor_set_layout_with_texture);

  // pe_vk_pipeline_create_layout(true, &pe_vk_pipeline_layout_skinned,
  //                              &pe_vk_descriptor_set_layout_skinned);

  pe_vk_pipelines_init();

  pe_vk_initialized = true;

  pe_vk_commands_pool_init();

  pe_vk_create_color_resources();

  pe_vk_create_depth_resources();

  pe_vk_framebuffer_create();

  pe_vk_command_init();

  pe_vk_semaphores_create();


  LOG("Vulkan intialize [OK]\n");
  return 0;
}
