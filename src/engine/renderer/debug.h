#ifndef DEBUG_VULKAN
#define DEBUG_VULKAN

#include <vulkan/vulkan.h>
#include <stdbool.h>

extern bool pe_vk_validation_layer_enable;

extern const char *validation_layers[];

extern VkDebugUtilsMessengerCreateInfoEXT debug_message_info;

void pe_vk_populate_messenger_debug_info(VkDebugUtilsMessengerCreateInfoEXT *);
void pe_vk_setup_debug_messenger();
void pe_vk_debug_end();

#endif
