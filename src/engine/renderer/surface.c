#include "surface.h"

#include "vulkan.h"
#include "vk_images.h"
#include "swap_chain.h"
//#include "../window.h"

#include <pway/pway.h>

#include "images_view.h"

VkImage pe_vk_color_image;
VkDeviceMemory pe_vk_color_memory;
VkImageView pe_vk_color_image_view;

VkSurfaceKHR vk_surface;

PTexture vk_color_image;

void pe_vk_create_surface() {

  //pway owns the wl_surface, vulkan just renders into it. no EGL context is
  //created, the raw wayland objects are all this needs. the other path is
  //is_drm_rendering, which has no surface at all
  // if (!is_wayland_window)
  //   return;

  VkWaylandSurfaceCreateInfoKHR surface_create_info = {
      .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
      .display = pway_display,
      .surface = pway_surface,
  };

  if (vkCreateWaylandSurfaceKHR(vk_instance, &surface_create_info, NULL,
                                &vk_surface) != VK_SUCCESS)
    fprintf(stderr, "Failed to create Vulkan Wayland surface!\n");
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

void pe_vk_set_viewport_and_sccisor(){
  //TODO update when recreate swap chain for handling resizing window

  VkOffset2D offset = {0, 0};

  scissor.extent = pe_vk_swch_extent;
  scissor.offset = offset;

  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)pe_vk_swch_extent.width;
  viewport.height = (float)pe_vk_swch_extent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

}

