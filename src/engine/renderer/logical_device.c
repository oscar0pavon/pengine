#include "logical_device.h"
#include "vulkan.h"

#include "debug.h"
#include "queues.h"

#include <math.h>


const char *devices_extensions[] = {
    VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
    VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME, // Core external memory functionality
    VK_EXT_EXTERNAL_MEMORY_DMA_BUF_EXTENSION_NAME, // Specifics for dma-buf/GBM
                                                   // import
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_EXT_EXTERNAL_MEMORY_DMA_BUF_EXTENSION_NAME,
    VK_EXT_IMAGE_DRM_FORMAT_MODIFIER_EXTENSION_NAME,
};

//1 when the device cannot filter anisotropically, which leaves it off
float pe_vk_max_anisotropy = 1.0f;

//a surface seen at a grazing angle, a road going away from the camera, covers
//a long thin run of texels, and plain mipmapping blurs it to the width of the
//run. 16 is as much as is worth asking for
#define ANISOTROPY_WANTED 16.0f

static void enable_anisotropy(VkPhysicalDeviceFeatures *enabled) {
  VkPhysicalDeviceFeatures supported;
  vkGetPhysicalDeviceFeatures(vk_physical_device, &supported);
  if (!supported.samplerAnisotropy)
    return;

  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(vk_physical_device, &properties);

  enabled->samplerAnisotropy = VK_TRUE;
  pe_vk_max_anisotropy = fminf(properties.limits.maxSamplerAnisotropy,
                               ANISOTROPY_WANTED);
}

int pe_vk_create_logical_device() {

  VkPhysicalDeviceFeatures features;
  ZERO(features);
  enable_anisotropy(&features);

  VkDeviceCreateInfo info;
  ZERO(info);
  info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  info.enabledLayerCount = 1;
  info.ppEnabledLayerNames = validation_layers;
  info.queueCreateInfoCount = 1;
  info.pQueueCreateInfos = queues_creates_infos;
  info.pEnabledFeatures = &features;

  info.enabledExtensionCount = sizeof(devices_extensions) /
                               sizeof(devices_extensions[0]);
  info.ppEnabledExtensionNames = devices_extensions;

  VKVALID(vkCreateDevice(vk_physical_device, &info, NULL, &vk_device),
          "Can't create vkphydevice")
}
