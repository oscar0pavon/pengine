#include "physical_devices.h"
#include "vulkan.h"

VkPhysicalDevice vk_physical_device;

void pe_vk_get_physical_device() {

  uint32_t devices_count = 0;

  vkEnumeratePhysicalDevices(vk_instance, &devices_count, NULL);

  if (devices_count == 0)
    LOG("Not devices compatibles\n");
  else
    printf("Devices count %i\n", devices_count);

  VkPhysicalDevice devices[devices_count];
  vkEnumeratePhysicalDevices(vk_instance, &devices_count, devices);

  vk_physical_device = devices[0];
  if (vk_physical_device == NULL) {
    printf("Can't assing device\n");
  }

  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(vk_physical_device,&properties);

  printf("Device: %s\n", properties.deviceName);
}
