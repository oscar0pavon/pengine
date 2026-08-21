#include "swap_chain.h"
#include "debug.h"
#include "engine/images.h"

#include "vulkan.h"
#include <engine/log.h>
#include <engine/macros.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

#include "images_view.h"

#include "engine/renderer/renderer.h"

#include "engine/camera.h"

VkSwapchainKHR pe_vk_swap_chain;

VkFormat pe_vk_swch_format;
VkExtent2D pe_vk_swch_extent;

u32 pe_vk_swapchain_image_count;

//TODO we use four images buffer we can use less
VkImage pe_vk_swch_images[PE_VK_MAX_SWAPCHAIN_IMAGES];
PTexture pe_vk_exportable_images[PE_VK_MAX_SWAPCHAIN_IMAGES];
int pe_vk_exportable_images_id[PE_VK_MAX_SWAPCHAIN_IMAGES];

typedef struct PSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  Array formats;
  Array present_modes;
} PSupportDetails;

PSupportDetails pe_vk_query_swap_chain_support(VkPhysicalDevice device) {
  PSupportDetails details;
  ZERO(details);
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_physical_device, vk_surface,
                                            &details.capabilities);

  uint32_t format_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR(vk_physical_device, vk_surface,
                                       &format_count, NULL);

  array_init(&details.formats, sizeof(VkSurfaceFormatKHR), format_count);
  details.formats.count = format_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR(vk_physical_device, vk_surface,
                                       &format_count, details.formats.data);

  uint32_t modes_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR(vk_physical_device, vk_surface,
                                            &modes_count, NULL);

  array_init(&details.present_modes, sizeof(VkPresentModeKHR), modes_count);
  details.present_modes.count = modes_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR(
      vk_physical_device, vk_surface, &modes_count, details.present_modes.data);

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
pe_vk_swch_choose_extent(const VkSurfaceCapabilitiesKHR *capabilities) {
  if (capabilities->currentExtent.width != UINT32_MAX)
    return capabilities->currentExtent;
  else {
    VkExtent2D current;
    if (is_drm_rendering) {
      current.width = 1920;
      current.height = 1080;
    } else {
      current.width = pe_window_width;
      current.height = pe_window_height;
    }
    return current;
  }
}

void pe_vk_create_swapchain() {
  if (vk_physical_device == NULL) {
    printf("ERROR None phisical device selected\n");
  }

  PSupportDetails support = pe_vk_query_swap_chain_support(vk_physical_device);
  VkSurfaceFormatKHR format =
      pe_vk_swch_choose_surface_format(&support.formats);
  VkPresentModeKHR mode =
      pe_vk_swch_choose_present_mode(&support.present_modes);
  VkExtent2D extent = pe_vk_swch_choose_extent(&support.capabilities);

  pe_vk_swapchain_image_count = support.capabilities.minImageCount + 1;

  VkSwapchainCreateInfoKHR info = {
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .surface = vk_surface,
      .minImageCount = pe_vk_swapchain_image_count,
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

  VKVALID(vkCreateSwapchainKHR(vk_device, &info, NULL, &pe_vk_swap_chain),
          "Can't create a swap schain");

  pe_vk_swch_extent = extent;
  pe_vk_swch_format = format.format;

  LOG("Swap chain extent %i, %i\n", pe_vk_swch_extent.width,
      pe_vk_swch_extent.height);
  
  u32 getting_images_count = 0;
  
  VKVALID(
      vkGetSwapchainImagesKHR(vk_device, pe_vk_swap_chain, &getting_images_count, NULL),
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

  VKVALID(vkGetSwapchainImagesKHR(vk_device, pe_vk_swap_chain, &getting_images_count,
                                  pe_vk_swch_images),
          "Cant't get images from swapchain");

  //everything sized per swapchain image - the image views, the descriptor
  //pool, the uniform buffers - counts from here, so it has to be the number
  //that exist rather than the number requested
  pe_vk_swapchain_image_count = getting_images_count;

  if (pe_vk_swch_images[0] == VK_NULL_HANDLE) {
    printf("Swapchain image not valid");
  }
}
