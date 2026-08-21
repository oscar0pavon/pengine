#include "display.h"

#include "vulkan.h"
//#include "window.h"
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>

VkDisplayKHR vk_display;
VkDisplayModeKHR vk_display_mode;

void vk_get_displays() {
  u32 display_count;

  vkGetPhysicalDeviceDisplayPropertiesKHR(vk_physical_device, &display_count,
                                          NULL);

  printf("Vulkan displays count: %i\n", display_count);
  if(display_count == 0){
    printf("None display detected. Exit\n");
    exit(1);
  }

  VkDisplayPropertiesKHR displays[display_count];
  vkGetPhysicalDeviceDisplayPropertiesKHR(vk_physical_device,
      &display_count,
      displays);
  
  vk_display = displays[0].display;

  u32 display_modes_count;
  vkGetDisplayModePropertiesKHR(vk_physical_device, vk_display, 
     &display_modes_count,NULL );
  
  printf("Display modes count %i\n", display_modes_count);

  VkDisplayModePropertiesKHR modes[display_modes_count];
  vkGetDisplayModePropertiesKHR(vk_physical_device, vk_display, 
     &display_modes_count,modes);

  vk_display_mode = modes[0].displayMode;
  
}
