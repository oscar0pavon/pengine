#include "instance.h"
#include "vulkan.h"

#include "debug.h"


const char *instance_extensions_names[] = {
    VK_KHR_DISPLAY_EXTENSION_NAME, 
    VK_KHR_SURFACE_EXTENSION_NAME, 
    VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME, 
    VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME, 
    };

void pe_vk_create_instance() {

  uint32_t instance_layer_properties_count = 0;
  vkEnumerateInstanceLayerProperties(&instance_layer_properties_count, NULL);
  LOG("VK instance layer count: %i\n", instance_layer_properties_count);

  VkLayerProperties layers_properties[instance_layer_properties_count];
  vkEnumerateInstanceLayerProperties(&instance_layer_properties_count,
                                     layers_properties);
  for (int i = 0; i < instance_layer_properties_count; i++) {
    //	LOG("%s\n",layers_properties[i].layerName);
  }

  VkApplicationInfo app_info = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pApplicationName = "swordfish",
      .applicationVersion = VK_MAKE_VERSION(1, 1, 0),
      .pEngineName = "swordfish_engine",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = VK_API_VERSION_1_3,
  };

  VkInstanceCreateInfo instance_info = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &app_info,
      .enabledExtensionCount = sizeof(instance_extensions_names) /
                               sizeof(instance_extensions_names[0]),
      .ppEnabledExtensionNames = instance_extensions_names,
  };

  if (pe_vk_validation_layer_enable == true) {

    instance_info.enabledLayerCount = 1;
    instance_info.ppEnabledLayerNames = validation_layers;

    ZERO(debug_message_info);
    pe_vk_populate_messenger_debug_info(&debug_message_info);
    instance_info.pNext = &debug_message_info;

  } else {
    instance_info.enabledLayerCount = 0;
  }

  VKVALID(vkCreateInstance(&instance_info, NULL, &vk_instance),
          "Can't create vk instance");

  if (pe_vk_validation_layer_enable == true) {
    pe_vk_setup_debug_messenger();
  }
}
