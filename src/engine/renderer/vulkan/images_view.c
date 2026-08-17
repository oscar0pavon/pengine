#include "images_view.h"
#include <engine/macros.h>
#include <stdint.h>
#include <string.h>
#include <engine/renderer/vulkan/swap_chain.h>
#include <engine/log.h>
#include <engine/macros.h>

#define PE_IMAGES_VIEWS_COUNT 4

VkImageView pe_vk_create_image_view(VkImage image, VkFormat format,
                                    VkImageAspectFlags aspect_flags,
                                    uint32_t mip_level) {
  VkImageViewCreateInfo viewInfo = {};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = image;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = format;
  viewInfo.subresourceRange.aspectMask = aspect_flags;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = mip_level;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;
  
  VkImageView image_view;

  vkCreateImageView(vk_device, &viewInfo, NULL, &image_view);
  
  return image_view;
}

void pe_vk_create_images_views() {
  array_init(&pe_vk_images_views, sizeof(VkImageView), PE_IMAGES_VIEWS_COUNT);

  // images view count equal to pe_vk_images array
  for (size_t i = 0; i < PE_IMAGES_VIEWS_COUNT; i++) {
    VkImageView image_view;
    ZERO(image_view);

    image_view = pe_vk_create_image_view(pe_vk_swch_images[i], pe_vk_swch_format,
        VK_IMAGE_ASPECT_COLOR_BIT,1);
    array_add(&pe_vk_images_views, &image_view);
  }
}
