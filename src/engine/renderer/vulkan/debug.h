#ifndef DEBUG_VULKAN
#define DEBUG_VULKAN
#include <vulkan/vulkan.h>

VkDebugUtilsMessengerCreateInfoEXT g_messenger_info;
void pe_vk_populate_messenger_debug_info(VkDebugUtilsMessengerCreateInfoEXT*);
void pe_vk_setup_debug_messenger();
void pe_vk_debug_end();
#endif
