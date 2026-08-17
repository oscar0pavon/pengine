#include "engine/images.h"
#include <engine/renderer/vulkan/vulkan.h>
#include <stdint.h>
#include <vulkan/vulkan_core.h>

VkImage pe_vk_texture_image;
VkImageView pe_vk_texture_image_view;
VkSampler pe_vk_texture_sampler;

VkImage pe_vk_depth_image;
VkDeviceMemory pe_vk_depth_image_memory;
VkImageView pe_vk_depth_image_view;

VkDeviceMemory pe_vk_texture_image_memory;

uint32_t pe_vk_mip_levels;

typedef struct PImageCreateInfo {
  uint32_t width;
  uint32_t height;
  uint32_t mip_level;
  VkFormat format;
  VkImageTiling tiling;
  VkImageUsageFlags usage;
  VkMemoryPropertyFlags properties;
  VkImage *texture_image;
  VkDeviceMemory *image_memory;
  VkSampleCountFlagBits number_of_samples;
} PImageCreateInfo;

void pe_vk_create_texture_image();

void pe_vk_create_image(PImageCreateInfo *info);
void pe_vk_create_depth_resources();
void pe_vk_create_texture_sampler();
