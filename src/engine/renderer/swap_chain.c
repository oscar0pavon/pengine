#include "swap_chain.h"
#include "debug.h"
#include "engine/images.h"

#include "engine/renderer/render_target.h"
#include "vulkan.h"
#include <engine/log.h>
#include <engine/macros.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

#include "commands.h"
#include "framebuffer.h"
#include "images_view.h"
#include "surface.h"
#include "sync.h"
#include "vk_images.h"

#include "engine/renderer/renderer.h"

#include "engine/camera.h"

typedef struct PSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  Array formats;
  Array present_modes;
} PSupportDetails;

PSupportDetails pe_vk_query_swap_chain_support(VkPhysicalDevice device,
                                               VkSurfaceKHR surface) {
  PSupportDetails details;
  ZERO(details);
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_physical_device, surface,
                                            &details.capabilities);

  uint32_t format_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR(vk_physical_device, surface,
                                       &format_count, NULL);

  array_init(&details.formats, sizeof(VkSurfaceFormatKHR), format_count);
  details.formats.count = format_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR(vk_physical_device, surface,
                                       &format_count, details.formats.data);

  uint32_t modes_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR(vk_physical_device, surface,
                                            &modes_count, NULL);

  array_init(&details.present_modes, sizeof(VkPresentModeKHR), modes_count);
  details.present_modes.count = modes_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR(
      vk_physical_device, surface, &modes_count, details.present_modes.data);

  return details;
}

VkSurfaceFormatKHR pe_vk_swch_choose_surface_format(Array *formats) {
  for (u8 i = 0; i < formats->count; i++) {
    VkSurfaceFormatKHR *format = array_get(formats, i);
    if (format->format == VK_FORMAT_B8G8R8A8_SRGB &&
        format->colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return *(format);
    }
  }
  VkSurfaceFormatKHR *format = array_get(formats, 0);
  return *(format);
}

VkPresentModeKHR pe_vk_swch_choose_present_mode(Array *present_modes) {
  for (u8 i = 0; i < present_modes->count; i++) {
    VkPresentModeKHR *mode = array_get(present_modes, i);
    if (*mode == VK_PRESENT_MODE_MAILBOX_KHR)
      return VK_PRESENT_MODE_MAILBOX_KHR;
  }
  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D
pe_vk_swch_choose_extent(PRenderTarget *target,
                         const VkSurfaceCapabilitiesKHR *capabilities) {
  if (capabilities->currentExtent.width != UINT32_MAX)
    return capabilities->currentExtent;
  else {
    VkExtent2D current;
    //INFO belt and braces: a display surface already defines currentExtent
    //from its chosen mode, so this almost never fires under DRM.
    //pe_vk_create_display_surface() fills target->width/heigth from that same
    //mode before this ever runs, so it is a safe fallback if it does
    if (is_drm_rendering) {
      current.width = target->width;
      current.height = target->heigth;
    } else {
      current.width = pe_window_width;
      current.height = pe_window_height;
    }

    //a client that picks its own extent still has to stay inside the range
    //the surface supports. a compositor can configure a window to a size the
    //swap chain would be rejected for, and a resize is where that shows up
    if (current.width < capabilities->minImageExtent.width)
      current.width = capabilities->minImageExtent.width;
    if (current.height < capabilities->minImageExtent.height)
      current.height = capabilities->minImageExtent.height;

    if (current.width > capabilities->maxImageExtent.width)
      current.width = capabilities->maxImageExtent.width;
    if (current.height > capabilities->maxImageExtent.height)
      current.height = capabilities->maxImageExtent.height;

    return current;
  }
}

void pe_vk_create_swapchain(PRenderTarget* target) {
  if (vk_physical_device == NULL) {
    printf("ERROR None phisical device selected\n");
  }

  PSupportDetails support =
      pe_vk_query_swap_chain_support(vk_physical_device, target->surface);
  VkSurfaceFormatKHR format =
      pe_vk_swch_choose_surface_format(&support.formats);
  VkPresentModeKHR mode =
      pe_vk_swch_choose_present_mode(&support.present_modes);
  VkExtent2D extent = pe_vk_swch_choose_extent(target, &support.capabilities);

  target->images_count = support.capabilities.minImageCount + 1;

  VkSwapchainCreateInfoKHR info = {
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .surface = target->surface,
      .minImageCount = target->images_count,
      .imageFormat = format.format,
      .presentMode = mode,
      .imageExtent = extent,
      .imageArrayLayers = 1,
      .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .preTransform = support.capabilities.currentTransform,
      .clipped = VK_FALSE,
      .oldSwapchain = VK_NULL_HANDLE};

  //INHERIT is what an xlib surface offers, a wayland one does not. ask the
  //surface instead of assuming: opaque if it is there, otherwise whatever
  //the lowest supported bit is
  if (support.capabilities.supportedCompositeAlpha &
      VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
    info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  else
    info.compositeAlpha = (VkCompositeAlphaFlagBitsKHR)(
        support.capabilities.supportedCompositeAlpha &
        -support.capabilities.supportedCompositeAlpha);

  VKVALID(vkCreateSwapchainKHR(vk_device, &info, NULL, &target->swap_chain),
          "Can't create a swap schain");

  target->extent = extent;
  target->format = format.format;
  target->width = extent.width;
  target->heigth = extent.height;

  LOG("Swap chain extent %i, %i\n", target->extent.width,
      target->extent.height);
  
  u32 getting_images_count = 0;
  
  VKVALID(
      vkGetSwapchainImagesKHR(vk_device, target->swap_chain, &getting_images_count, NULL),
      "Can't get swap chain images");

  printf("Getting %i swapchain images\n", getting_images_count);

  //the count the driver reports is what the second call writes, and it is not
  //the minImageCount asked for above - mesa gave 5 here for a requested 4. it
  //used to be written straight into a four element array, and the two bytes
  //past the end were pe_vk_swap_chain itself: the handle was overwritten with
  //an image the moment the swapchain was created, and the next acquire took
  //the whole process down inside the driver
  if (getting_images_count > PE_VK_MAX_SWAPCHAIN_IMAGES)
    getting_images_count = PE_VK_MAX_SWAPCHAIN_IMAGES;

  VKVALID(vkGetSwapchainImagesKHR(vk_device, target->swap_chain, &getting_images_count,
                                  target->swap_chain_images),
          "Cant't get images from swapchain");

  //everything sized per swapchain image - the image views, the descriptor
  //pool, the uniform buffers - counts from here, so it has to be the number
  //that exist rather than the number requested
  target->images_count = getting_images_count;

  if (target->swap_chain_images[0] == VK_NULL_HANDLE) {
    printf("Swapchain image not valid");
  }
}

//everything that was sized from the old extent, in the reverse of the order
//pe_vk_init() built it
static void pe_vk_destroy_swapchain_resources(PRenderTarget *target) {

  for (int i = 0; i < target->framebuffers.count; i++) {
    VkFramebuffer *framebuffer = array_get(&target->framebuffers, i);
    vkDestroyFramebuffer(vk_device, *framebuffer, NULL);
  }

  vkDestroyImageView(vk_device, target->depth_image_view, NULL);
  vkDestroyImage(vk_device, target->depth_image, NULL);
  vkFreeMemory(vk_device, target->depth_memory, NULL);

  vkDestroyImageView(vk_device, target->color_image_view, NULL);
  vkDestroyImage(vk_device, target->color_image, NULL);
  vkFreeMemory(vk_device, target->color_memory, NULL);

  for (int i = 0; i < target->images_views.count; i++) {
    VkImageView *image_view = array_get(&target->images_views, i);
    vkDestroyImageView(vk_device, *image_view, NULL);
  }

  vkDestroySwapchainKHR(vk_device, target->swap_chain, NULL);
}

void pe_vk_recreate_swapchain(PRenderTarget *target) {

  //nothing below this can be destroyed while a frame in flight is still
  //reading it, and there is no other point where that is known
  vkDeviceWaitIdle(vk_device);

  u32 previous_images_count = target->images_count;

  //the command buffers and the render finished semaphores are counted per
  //swap chain image, so they belong to the swap chain that is going away
  vkFreeCommandBuffers(vk_device, target->commands_pool,
                       target->command_buffers.count,
                       target->command_buffers.data);

  pe_vk_end_sync(target);

  pe_vk_destroy_swapchain_resources(target);

  pe_vk_create_swapchain(target);
  pe_vk_set_viewport_and_sccisor(target);
  pe_vk_create_images_views(target);
  pe_vk_create_color_resources(target);
  pe_vk_create_depth_resources(target);
  pe_vk_framebuffer_create(target);
  pe_vk_command_init(target);
  pe_vk_semaphores_create(target);

  //INFO the render pass, the pipelines and the layouts all survive: the
  //format has not changed, and the viewport and scissor are dynamic state
  //that pe_vk_draw_commands() sets from the target every frame

  //INFO a model's uniform buffers and descriptor sets are one per swap chain
  //image and were sized when the model was created. more images than there
  //were then and image_index runs off the end of both
  if (target->images_count != previous_images_count)
    LOG("Swap chain came back with %i images instead of %i - models built "
        "before it have descriptor sets for the old count\n",
        target->images_count, previous_images_count);

  camera_init_with_size(&target->camera, target->width, target->heigth);
}
