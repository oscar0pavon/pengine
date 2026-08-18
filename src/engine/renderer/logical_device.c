#include "logical_device.h"
#include "vulkan.h"

#include "debug.h"
#include "queues.h"


const char *devices_extensions[] = {
    VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
    VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME, // Core external memory functionality
    VK_EXT_EXTERNAL_MEMORY_DMA_BUF_EXTENSION_NAME, // Specifics for dma-buf/GBM
                                                   // import
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_EXT_EXTERNAL_MEMORY_DMA_BUF_EXTENSION_NAME,
    VK_EXT_IMAGE_DRM_FORMAT_MODIFIER_EXTENSION_NAME,
};

int pe_vk_create_logical_device() {

  VkDeviceCreateInfo info;
  ZERO(info);
  info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  info.enabledLayerCount = 1;
  info.ppEnabledLayerNames = validation_layers;
  info.queueCreateInfoCount = 1;
  info.pQueueCreateInfos = queues_creates_infos;

  info.enabledExtensionCount = sizeof(devices_extensions) /
                               sizeof(devices_extensions[0]);
  info.ppEnabledExtensionNames = devices_extensions;

  VKVALID(vkCreateDevice(vk_physical_device, &info, NULL, &vk_device),
          "Can't create vkphydevice")
}
