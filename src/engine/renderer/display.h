#ifndef DISPLAY_H
#define DISPLAY_H

#include <vulkan/vulkan_core.h>
#include "render_target.h"

typedef struct PVkDisplay {
  VkDisplayKHR display;
  VkDisplayModeKHR mode;
  VkExtent2D extent; //the chosen mode's visibleRegion
  u32 plane_index;
  u32 plane_stack_index; //the assigned plane's currentStackIndex
} PVkDisplay;

extern PVkDisplay pe_vk_displays[PE_VK_MAX_RENDER_TARGETS];
extern u32 pe_vk_displays_count;

void vk_get_displays();

#endif
