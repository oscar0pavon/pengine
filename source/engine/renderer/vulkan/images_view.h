#include <engine/renderer/vulkan/vulkan.h>

Array pe_vk_images_views;


void pe_vk_create_images_views();
VkImageView pe_vk_create_image_view(VkImage image, VkFormat format,
                                    VkImageAspectFlags aspect_flags);
