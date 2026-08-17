
#include <engine/macros.h>
#include <engine/renderer/vulkan/vulkan.h>
#include <engine/renderer/vulkan/framebuffer.h>
#include <engine/renderer/vulkan/images_view.h>
#include "swap_chain.h"

#include "vk_images.h"

void pe_vk_framebuffer_create() {

  array_init(&pe_vk_framebuffers, sizeof(VkFramebuffer),
             pe_vk_images_views.count);

  for (int i = 0; i < pe_vk_images_views.count; i++) {

    VkImageView *framebuffer_image_view = array_get(&pe_vk_images_views, i);

    VkImageView attachments[3];
    attachments[2] = *(framebuffer_image_view);
    attachments[1] = pe_vk_depth_image_view;
    attachments[0] = pe_vk_color_image_view;

    VkFramebufferCreateInfo info;
    ZERO(info);
    info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    info.renderPass = pe_vk_render_pass;
    info.attachmentCount = 3;
    info.pAttachments = attachments;
    info.width = pe_vk_swch_extent.width;
    info.height = pe_vk_swch_extent.height;
    info.layers = 1;

    VkCommandBuffer buffer;
    vkCreateFramebuffer(vk_device, &info, NULL, &buffer);
    array_add(&pe_vk_framebuffers, &buffer);

  }
}
