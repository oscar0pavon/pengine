#include "vulkan.h"
#include <stdint.h>
#include <engine/array.h>
#include "render_target.h"
extern Array pe_vk_images_views;


void pe_vk_create_images_views(PRenderTarget *target);

VkImageView pe_vk_create_image_view(VkImage image, VkFormat format,
                                    VkImageAspectFlags aspect_flags,
                                    uint32_t mip_level);
