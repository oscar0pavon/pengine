#ifndef VK_IMAGES_H
#define VK_IMAGES_H

#include "engine/images.h"
#include "vulkan.h"
#include <stdint.h>
#include <vulkan/vulkan_core.h>

typedef struct PImportImageInfo{
  VkImportMemoryFdInfoKHR info; 
  uint32_t file_descriptor;
}PImportImageInfo;

typedef struct PImageCreateInfo {
  uint32_t width;
  uint32_t height;
  uint32_t mip_level;
  VkFormat format;
  VkImageTiling tiling;
  VkImageUsageFlags usage;
  VkMemoryPropertyFlags properties;
  PTexture* texture;
  VkSampleCountFlagBits number_of_samples;
  bool is_exportable;
  bool is_importable;
  PImportImageInfo import;
  const void *pNext;
} PImageCreateInfo;

extern VkImageView pe_vk_depth_image_view;

extern PTexture vk_depth_image;

void pe_vk_import_image(PTexture *new_texture, uint32_t witdh, uint32_t height,
                        uint32_t file_descriptor, uint64_t modifier);

void pe_vk_create_texture(PTexture* new_texture, const char* path);

void pe_vk_clean_image(PTexture* image);
void pe_vk_create_image(PImageCreateInfo *info);
void pe_vk_create_depth_resources();

void pe_vk_create_exportable_images();

void pe_vk_image_to_color_attacthment(VkImage image);

void pe_vk_image_color_to_transfer(VkImage image);

void pe_vk_image_to_destination(VkImage image);

void pe_vk_copy_image(VkImage source, VkImage destination);

void pe_vk_image_transfer_to_present(VkImage image);

#endif
