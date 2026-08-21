#ifndef DISPLAY_H
#define DISPLAY_H

#include <vulkan/vulkan_core.h>

void vk_get_displays();

extern VkDisplayKHR vk_display;
extern VkDisplayModeKHR vk_display_mode;

#endif
