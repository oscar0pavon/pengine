#include "debug.h"
#include "vulkan.h"
#include <engine/log.h>

VkDebugUtilsMessengerEXT debug_messenger;

static VKAPI_ATTR VkBool32 VKAPI_CALL
pe_vk_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                     VkDebugUtilsMessageTypeFlagsEXT message_type,
                     const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
                     void *user_data) {

  //LOG("Vulkan: %s\n", callback_data->pMessage);
  char *message = callback_data->pMessage;
  int char_count = 0;
  int char_position = 0;
  for (char_position = 0; message[char_position]; char_position++) {
    if (message[char_position] == ':') {
    }
    char_count++;
  }
  char new_message[char_count + 20];
  ZERO(new_message);
  //  LOG("Message lengh = %i", char_count);
  for (int i = 0; i < char_count; i++) {
    new_message[i] = message[i];
  }
  LOG("%s\n\n\n", new_message);
  return VK_FALSE;
}

void pe_vk_destroy_debug_utils(VkInstance instance,
                               VkDebugUtilsMessengerEXT debug_msg,
                               const VkAllocationCallbacks *alloc) {
  PFN_vkDestroyDebugUtilsMessengerEXT func =
      vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func)
    func(instance, debug_msg, alloc);
}

VkResult
pe_vk_create_debug_messeger(VkInstance instance,
                            const VkDebugUtilsMessengerCreateInfoEXT *info,
                            const VkAllocationCallbacks *allocator,
                            VkDebugUtilsMessengerEXT *debug_messeger) {

  PFN_vkCreateDebugUtilsMessengerEXT func =
      (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
          instance, "vkCreateDebugUtilsMessengerEXT");

  if (func != NULL) {
    return func(instance, info, allocator, debug_messeger);
  } else {
    LOG("Cant't create debug messenger\n");
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}
void pe_vk_populate_messenger_debug_info(
    VkDebugUtilsMessengerCreateInfoEXT *info_messeger) {

  info_messeger->sType =
      VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  // info_messeger->messageSeverity =
  // VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
  //												VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
  //| 												VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  info_messeger->messageSeverity =
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  info_messeger->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  info_messeger->pfnUserCallback = &pe_vk_debug_callback;
  info_messeger->pUserData = NULL;
}

void pe_vk_setup_debug_messenger() {

  pe_vk_create_debug_messeger(vk_instance, &g_messenger_info, NULL,
                              &debug_messenger);
}

void pe_vk_debug_end() {
  pe_vk_destroy_debug_utils(vk_instance, debug_messenger, NULL);
}
