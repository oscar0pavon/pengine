#include "display.h"

#include "vulkan.h"
//#include "window.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan_core.h>

PVkDisplay pe_vk_displays[PE_VK_MAX_RENDER_TARGETS];
u32 pe_vk_displays_count;

//the mode the compositor doesn't have to fight the driver over - the one
//whose visible region matches the display's own physical resolution.
//falls back to modes[0] if nothing matches
static VkDisplayModePropertiesKHR
pick_native_mode(VkDisplayModePropertiesKHR *modes, u32 count,
                 VkExtent2D physical_resolution) {
  for (u32 i = 0; i < count; i++) {
    VkExtent2D region = modes[i].parameters.visibleRegion;
    if (region.width == physical_resolution.width &&
        region.height == physical_resolution.height)
      return modes[i];
  }
  return modes[0];
}

//the first plane, among however many the device has, that (a) nothing else
//has claimed yet and (b) vkGetDisplayPlaneSupportedDisplaysKHR says can scan
//out this display. returns false if none exists
static bool
find_free_plane(VkDisplayKHR display, u32 plane_count, bool *plane_used,
                u32 *out_plane_index) {
  for (u32 p = 0; p < plane_count; p++) {
    if (plane_used[p])
      continue;

    u32 supported_count;
    vkGetDisplayPlaneSupportedDisplaysKHR(vk_physical_device, p,
                                          &supported_count, NULL);
    if (supported_count == 0)
      continue;

    VkDisplayKHR supported[supported_count];
    vkGetDisplayPlaneSupportedDisplaysKHR(vk_physical_device, p,
                                          &supported_count, supported);

    for (u32 s = 0; s < supported_count; s++) {
      if (supported[s] == display) {
        *out_plane_index = p;
        return true;
      }
    }
  }
  return false;
}

void vk_get_displays() {
  u32 display_count;

  vkGetPhysicalDeviceDisplayPropertiesKHR(vk_physical_device, &display_count,
                                          NULL);

  printf("Vulkan displays count: %i\n", display_count);
  if(display_count == 0){
    printf("None display detected. Exit\n");
    exit(1);
  }

  //INFO the registry (PRenderTarget array) has a fixed size - a box with more
  //physical outputs than that just doesn't get the rest driven
  if (display_count > PE_VK_MAX_RENDER_TARGETS)
    display_count = PE_VK_MAX_RENDER_TARGETS;

  VkDisplayPropertiesKHR displays[display_count];
  vkGetPhysicalDeviceDisplayPropertiesKHR(vk_physical_device,
      &display_count,
      displays);

  u32 plane_count;
  vkGetPhysicalDeviceDisplayPlanePropertiesKHR(vk_physical_device,
      &plane_count, NULL);
  VkDisplayPlanePropertiesKHR planes[plane_count];
  vkGetPhysicalDeviceDisplayPlanePropertiesKHR(vk_physical_device,
      &plane_count, planes);

  bool plane_used[plane_count];
  ZERO(plane_used);

  pe_vk_displays_count = 0;

  for (u32 i = 0; i < display_count; i++) {
    VkDisplayKHR display = displays[i].display;

    u32 display_modes_count;
    vkGetDisplayModePropertiesKHR(vk_physical_device, display,
        &display_modes_count, NULL);

    if (display_modes_count == 0) {
      printf("Display %i (%s) has no modes, skipping\n", i,
             displays[i].displayName);
      continue;
    }

    VkDisplayModePropertiesKHR modes[display_modes_count];
    vkGetDisplayModePropertiesKHR(vk_physical_device, display,
        &display_modes_count, modes);

    VkDisplayModePropertiesKHR chosen = pick_native_mode(
        modes, display_modes_count, displays[i].physicalResolution);

    u32 plane_index;
    if (!find_free_plane(display, plane_count, plane_used, &plane_index)) {
      printf("Display %i (%s) has no free supported plane, skipping\n", i,
             displays[i].displayName);
      continue;
    }
    plane_used[plane_index] = true;

    PVkDisplay *out = &pe_vk_displays[pe_vk_displays_count];
    out->display = display;
    out->mode = chosen.displayMode;
    out->extent = chosen.parameters.visibleRegion;
    out->plane_index = plane_index;
    out->plane_stack_index = planes[plane_index].currentStackIndex;

    printf("Display %i: %s %ix%i@%.1fHz, plane %i\n", pe_vk_displays_count,
           displays[i].displayName, out->extent.width, out->extent.height,
           chosen.parameters.refreshRate / 1000.f, plane_index);

    pe_vk_displays_count++;
  }

  if (pe_vk_displays_count == 0) {
    printf("No display had both a mode and a free plane. Exit\n");
    exit(1);
  }
}
