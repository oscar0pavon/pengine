#ifndef RENDER_PASS_H
#define RENDER_PASS_H

#include <vulkan/vulkan.h>
#include "render_target.h"

//INFO the render pass is one object shared by every target - format is the
//only thing it reads off a target, and the first-cut assumption is that every
//target shares one format
void pe_vk_create_render_pass(PRenderTarget *target);

void pe_vk_start_render_pass(PRenderTarget *target, VkCommandBuffer command,
                             int i);

#endif
