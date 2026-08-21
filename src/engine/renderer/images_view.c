#include "images_view.h"
#include "swap_chain.h"
#include <engine/log.h>
#include <engine/macros.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

Array pe_vk_images_views;

VkImageView pe_vk_create_image_view(VkImage image, VkFormat format,
                                    VkImageAspectFlags aspect_flags,
                                    uint32_t mip_level) {
  if (image == VK_NULL_HANDLE) {
    printf("ERROR Image null\n");
  }
  VkImageViewCreateInfo viewInfo = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = format,
      .subresourceRange.aspectMask = aspect_flags,
      .subresourceRange.baseMipLevel = 0,
      .subresourceRange.levelCount = mip_level,
      .subresourceRange.baseArrayLayer = 0,
      .subresourceRange.layerCount = 1};

  VkImageView image_view;

  VKVALID(vkCreateImageView(vk_device, &viewInfo, NULL, &image_view),
          "Can't create image view");

  // printf("Creating image view %p\n", image_view);

  return image_view;
}

void pe_vk_create_images_views(PRenderTarget *target) {
  array_init(&target->images_views, sizeof(VkImageView), target->images_count);

  // images view count equal to pe_vk_images array
  for (size_t i = 0; i < target->images_count; i++) {
    VkImageView image_view;

    image_view =
        pe_vk_create_image_view(target->swap_chain_images[i], pe_vk_swch_format,
                                VK_IMAGE_ASPECT_COLOR_BIT, 1);

    array_add(&target->images_views, &image_view);
  }
}
