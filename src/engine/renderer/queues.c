#include "queues.h"
#include "vulkan.h"

VkDeviceQueueCreateInfo queues_creates_infos[2];

VkQueue vk_queue;

const float queue_priority = 1.f;

uint32_t q_graphic_family;
uint32_t q_present_family;

void pe_vk_queue_families_support() {

  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(vk_physical_device,
                                           &queue_family_count, NULL);
  LOG("Queue families count: %i\n", queue_family_count);

  VkQueueFamilyProperties q_families[queue_family_count];
  ZERO(q_families);
  vkGetPhysicalDeviceQueueFamilyProperties(vk_physical_device,
                                           &queue_family_count, q_families);
  for (int i = 0; i < queue_family_count; i++) {
    VkQueueFamilyProperties property = q_families[i];
    // LOG("Family queue flag %x", property.queueFlags);
    if (property.queueFlags == VK_QUEUE_GRAPHICS_BIT) {
      q_graphic_family = i;

      LOG("graphics queue found");
    } else {

      // LOG("[X] No graphics queue found\n");
    }

    // VkBool32 present_support = false;
    // vkGetPhysicalDeviceSurfaceSupportKHR(vk_physical_device, i, vk_surface,
    //                                      &present_support);
    // if (present_support == true)
    //   q_present_family = i;
    // else {
    //
    //   // LOG("[X] NO present queue found");
    // }
  }

  ZERO(queues_creates_infos);
  uint32_t q_unique_falimiles[] = {q_graphic_family, q_present_family};
  for (uint32_t i = 0; i < 2; i++) {
    VkDeviceQueueCreateInfo *info = &queues_creates_infos[i];
    info->sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    info->queueFamilyIndex = q_unique_falimiles[i];
    info->queueCount = 1;
    info->pQueuePriorities = &queue_priority;
  }
}
