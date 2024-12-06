#include "vulkan.h"
#include "commands.h"
#include "debug.h"
#include "descriptor_set.h"
#include "engine/engine.h"
#include "framebuffer.h"
#include "images_view.h"
#include "pipeline.h"
#include "render_pass.h"
#include "sync.h"
#include "vk_images.h"
#include "vk_vertex.h"
#include <engine/game.h>
#include <engine/log.h>
#include <engine/macros.h>
#include <engine/renderer/vulkan/descriptor_set.h>
#include <engine/renderer/vulkan/shader_module.h>
#include <engine/renderer/vulkan/swap_chain.h>
#include <engine/renderer/vulkan/uniform_buffer.h>
#include <stdint.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

const char *validation_layers[] = {"VK_LAYER_KHRONOS_validation"};
// For Linux support
#ifdef DESKTOP
const char *instance_extension[] = {"VK_KHR_surface", "VK_KHR_xcb_surface",
                                    VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
#endif
// For Android support
#ifdef ANDROID
const char *instance_extension[] = {"VK_KHR_surface", "VK_KHR_android_surface",
                                    VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
#endif
const char *devices_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

VkDeviceQueueCreateInfo queues_creates_infos[2];

const float queue_priority = 1.f;

int pe_vk_create_logical_device() {

  VkDeviceCreateInfo info;
  ZERO(info);
  info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  info.enabledLayerCount = 1;
  info.ppEnabledLayerNames = validation_layers;
  info.queueCreateInfoCount = 1;
  info.pQueueCreateInfos = queues_creates_infos;

  info.enabledExtensionCount = 1;
  info.ppEnabledExtensionNames = devices_extensions;
  VKVALID(vkCreateDevice(vk_physical_device, &info, NULL, &vk_device),
          "Can't create vkphydevice")
}

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
    LOG("Family queue flag %x", property.queueFlags);
    if (property.queueFlags == VK_QUEUE_GRAPHICS_BIT) {
      q_graphic_family = i;

      LOG("graphics queue found");
    } else
      LOG("[X] No graphics queue found\n");

    VkBool32 present_support = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(vk_physical_device, i, vk_surface,
                                         &present_support);
    if (present_support == true)
      q_present_family = i;
    else
      LOG("[X] NO present queue found");
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

void pe_vk_create_instance() {

  pe_vk_validation_layer_enable = false;

  uint32_t instance_layer_properties_count = 0;
  vkEnumerateInstanceLayerProperties(&instance_layer_properties_count, NULL);
  LOG("VK instance layer count: %i\n", instance_layer_properties_count);

  VkLayerProperties layers_properties[instance_layer_properties_count];
  vkEnumerateInstanceLayerProperties(&instance_layer_properties_count,
                                     layers_properties);
  for (int i = 0; i < instance_layer_properties_count; i++) {
    //	LOG("%s\n",layers_properties[i].layerName);
  }

  VkApplicationInfo app_info;
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.apiVersion = VK_API_VERSION_1_0;
  app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.pApplicationName = "PavonVKRender";
  app_info.pNext = NULL;
  app_info.pEngineName = "PavonEngine";

  VkInstanceCreateInfo instance_info;
  ZERO(instance_info);
  instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instance_info.pApplicationInfo = &app_info;
  instance_info.enabledExtensionCount = 3;
  instance_info.ppEnabledExtensionNames = instance_extension;

  if (pe_vk_validation_layer_enable == true) {

    instance_info.enabledLayerCount = 1;
    instance_info.ppEnabledLayerNames = validation_layers;
    ZERO(g_messenger_info);
    pe_vk_populate_messenger_debug_info(&g_messenger_info);
    instance_info.pNext =
        (VkDebugUtilsMessengerCreateInfoEXT *)&g_messenger_info;

  } else {
    instance_info.enabledLayerCount = 0;
  }

  VKVALID(vkCreateInstance(&instance_info, NULL, &vk_instance),
          "Can't create vk instance");
  //  VkResult r = vkCreateInstance(&instance_info, NULL, &vk_instance);
  // LOG("ERROR enum = %i", r);

  if (pe_vk_validation_layer_enable == true) {
    pe_vk_setup_debug_messenger();
  }
}
void pe_vk_get_physical_device() {

  //****************
  // Physical devices
  //****************
  uint32_t devices_count = 0;
  vkEnumeratePhysicalDevices(vk_instance, &devices_count, NULL);
  if (devices_count == 0)
    LOG("Not devices compatibles\n");
  VkPhysicalDevice phy_devices[devices_count];
  vkEnumeratePhysicalDevices(vk_instance, &devices_count, phy_devices);
  vk_physical_device = phy_devices[0];
}

void pe_vk_create_surface() {
#ifdef ANDROID
  VkAndroidSurfaceCreateInfoKHR surface_info;
  ZERO(surface_info);
  surface_info.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
  surface_info.pNext = NULL;
  surface_info.flags = 0;
  surface_info.window = game->app->window;
  if (game->app->window == NULL) {
    LOG("Window null");
  }
  VKVALID(
      vkCreateAndroidSurfaceKHR(vk_instance, &surface_info, NULL, &vk_surface),
      "Surface Error");
#endif
#if DESKTOP
  if (!current_window) {
    LOGW("NO WINDOWS CREATED");
  }

  VKVALID(glfwCreateWindowSurface(vk_instance, current_window->window, NULL,
                                  &vk_surface),
          "Can't create window surface");
#endif
}

void pe_vk_create_color_resources() {
  VkFormat color_format = pe_vk_swch_format;

  PImageCreateInfo image_create_info = {
      .width = pe_vk_swch_extent.width,
      .height = pe_vk_swch_extent.height,
      .texture_image = &pe_vk_color_image,
      .image_memory = &pe_vk_color_memory,
      .format = color_format,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
               VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      .mip_level = 1,
      .number_of_samples = pe_vk_msaa_samples};

  pe_vk_create_image(&image_create_info);

  pe_vk_color_image_view = pe_vk_create_image_view(
      pe_vk_color_image, color_format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

int pe_vk_init() {
  pe_vk_msaa_samples = VK_SAMPLE_COUNT_4_BIT;

  pe_vk_validation_layer_enable = true;

  pe_vk_create_instance();

  pe_vk_create_surface();

  pe_vk_get_physical_device();

  pe_vk_queue_families_support();

  pe_vk_create_logical_device();

  vkGetDeviceQueue(vk_device, q_graphic_family, 0, &vk_queue);

  pe_vk_swch_create();

  pe_vk_create_images_views();

  pe_vk_create_render_pass();

  pe_vk_create_descriptor_set_layout();
  pe_vk_create_descriptor_set_layout_with_texture();
  pe_vk_create_descriptor_set_layout_skinned();

  pe_vk_pipeline_create_layout(false, &pe_vk_pipeline_layout,
                               &pe_vk_descriptor_set_layout_with_texture);

  pe_vk_pipeline_create_layout(true, &pe_vk_pipeline_layout_with_descriptors,
                               &pe_vk_descriptor_set_layout_with_texture);

  pe_vk_pipeline_create_layout(true, &pe_vk_pipeline_layout_skinned,
                               &pe_vk_descriptor_set_layout_skinned);

  pe_vk_pipelines_init();

  pe_vk_initialized = true;

  pe_vk_commands_pool_init();

  pe_vk_create_color_resources();

  pe_vk_create_depth_resources();

  pe_vk_framebuffer_create();

  pe_vk_command_init();

  pe_vk_semaphores_create();

  pe_vk_create_texture_image();

  pe_vk_models_create();

  LOG("Vulkan intialize [OK]\n");
  return 0;
}

void pe_vk_end() {
  pe_vk_debug_end();
  vkDestroySurfaceKHR(vk_instance, vk_surface, NULL);
  vkDestroyDevice(vk_device, NULL);
  vkDestroyInstance(vk_instance, NULL);
}
