
#include <engine/macros.h>
#include "vulkan.h"
#include "framebuffer.h"
#include "images_view.h"
#include "swap_chain.h"

#include "vk_images.h"

void pe_vk_framebuffer_create(PRenderTarget *target) {

  array_init(&target->framebuffers, sizeof(VkFramebuffer),
             target->images_views.count);

  for (int i = 0; i < target->images_views.count; i++) {

    VkImageView *framebuffer_image_view = array_get(&target->images_views, i);

    VkImageView attachments[3];
    attachments[2] = *(framebuffer_image_view);
    attachments[1] = target->depth_image_view;
    attachments[0] = target->color_image_view;

    VkFramebufferCreateInfo info;
    ZERO(info);
    info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    info.renderPass = pe_vk_render_pass;
    info.attachmentCount = 3;
    info.pAttachments = attachments;
    info.width = target->extent.width;
    info.height = target->extent.height;
    info.layers = 1;

    VkFramebuffer buffer;
    vkCreateFramebuffer(vk_device, &info, NULL, &buffer);
    array_add(&target->framebuffers, &buffer);

  }
}
