#include "surface.h"

#include "engine/renderer/render_thread.h"
#include "vulkan.h"
#include "vk_images.h"
#include "swap_chain.h"
#include "renderer.h"

#include <pway/pway.h>

#include "images_view.h"
#include "display.h"
#include <vulkan/vulkan_wayland.h>

VkImage pe_vk_color_image;
VkDeviceMemory pe_vk_color_memory;
VkImageView pe_vk_color_image_view;

VkSurfaceKHR vk_surface;

PTexture vk_color_image;

void pe_vk_create_surface(PRenderTarget* target) {

  if(is_drm_rendering){
    pe_vk_create_display_surface(target);
    return;
  }

  VkWaylandSurfaceCreateInfoKHR surface_create_info = {
      .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
      .display = pway_display,
      .surface = pway_surface,
  };

  if (vkCreateWaylandSurfaceKHR(vk_instance, &surface_create_info, NULL,
                                &target->surface) != VK_SUCCESS)
    fprintf(stderr, "Failed to create Vulkan Wayland surface!\n");
}

void pe_vk_create_display_surface(PRenderTarget* target){
  VkDisplaySurfaceCreateInfoKHR info = {
    .sType = VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR, 
    .displayMode = vk_display_mode,
    .planeIndex = 0,
    .globalAlpha = 1.0f,
    .alphaMode = VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR,
    .planeStackIndex = 0, 
    .imageExtent = {1920,1080},
    .transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR
  };

  VKVALID(vkCreateDisplayPlaneSurfaceKHR(vk_instance,
      &info, NULL, &target->surface), "Can't create display surface");
  
}

void pe_vk_create_color_resources() {
  VkFormat color_format = pe_vk_swch_format;

  vk_color_image.mip_level = 1;
  PImageCreateInfo image_create_info = {
      .width = pe_vk_swch_extent.width,
      .height = pe_vk_swch_extent.height,
      .texture = &vk_color_image,
      .format = color_format,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
               VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      .mip_level = 1,
      .number_of_samples = pe_vk_msaa_samples};

  pe_vk_create_image(&image_create_info);

  pe_vk_color_image_view = pe_vk_create_image_view(
      vk_color_image.image, color_format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

void pe_vk_set_viewport_and_sccisor(PRenderTarget* target){
  //TODO update when recreate swap chain for handling resizing window

  VkOffset2D offset = {0, 0};

  target->scissor.extent = target->extent;
  target->scissor.offset = offset;

  target->viewport.x = 0.0f;
  target->viewport.y = 0.0f;
  target->viewport.width = (float)target->extent.width;
  target->viewport.height = (float)target->extent.height;
  target->viewport.minDepth = 0.0f;
  target->viewport.maxDepth = 1.0f;

}

