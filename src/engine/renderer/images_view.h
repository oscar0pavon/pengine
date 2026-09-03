#ifndef PE_VK_IMAGES_VIEW_H
#define PE_VK_IMAGES_VIEW_H

#include "vulkan.h"
#include <stdint.h>
#include <engine/array.h>
#include "render_target.h"

void pe_vk_create_images_views(PRenderTarget *target);

VkImageView pe_vk_create_image_view(VkImage image, VkFormat format,
                                    VkImageAspectFlags aspect_flags,
                                    uint32_t mip_level);

#endif // !PE_VK_IMAGES_VIEW_H
