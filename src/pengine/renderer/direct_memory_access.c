#include "direct_memory_access.h"
#include <vulkan/vulkan_core.h>

VkFormat find_compatible_external_format(VkPhysicalDevice physicalDevice) {
  // We are looking for a standard 8-bit RGBA format that supports external
  // memory
  VkFormat potential_formats[] = {
    VK_FORMAT_R8G8B8A8_UNORM,
    VK_FORMAT_B8G8R8A8_UNORM,
    VK_FORMAT_A8B8G8R8_UNORM_PACK32,

    // 10-bit formats
    VK_FORMAT_A2B10G10R10_UNORM_PACK32,
    VK_FORMAT_A2R10G10B10_UNORM_PACK32,
  };

  VkPhysicalDeviceExternalImageFormatInfo external_info = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO,
      .handleType =
          VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT, // The handle type you
                                                          // use
  };

  VkPhysicalDeviceImageFormatInfo2 format_info = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2,
      .pNext = &external_info, // Link the external memory info here
      .tiling = VK_IMAGE_TILING_LINEAR, // Or linear, depending on what works
      .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      .flags = 0,
      .type = VK_IMAGE_TYPE_2D
  };

  VkExternalImageFormatProperties external_props = {
      .sType = VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES,
  };

  VkImageFormatProperties2 props = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2,
      .pNext = &external_props,
  };

  for (int i = 0; i < sizeof(potential_formats) / sizeof(VkFormat); i++) {
    format_info.format = potential_formats[i];

    VkResult result = vkGetPhysicalDeviceImageFormatProperties2(
        physicalDevice, &format_info, &props);

    if (result == VK_SUCCESS &&
        (external_props.externalMemoryProperties.externalMemoryFeatures &
         VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT)) {
      // Found a compatible format that supports DMA-BUF import!
      return potential_formats[i];
    }
  }

  // If no format is found, crash with a fatal error
  fprintf(
      stderr,
      "Fatal Error: No compatible Vulkan format found for DMA-BUF import.\n");
  exit(EXIT_FAILURE);
}
