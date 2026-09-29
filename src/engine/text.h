#ifndef PE_TEXT_H
#define PE_TEXT_H

#include <cglm/cglm.h>
#include <vulkan/vulkan_core.h>

#include "renderer/render_target.h"

/* Text rendering on top of pfonts' Vulkan backend (/root/pfonts,
 * pfonts/vulkan.h): pfonts owns font loading, the glyph atlas and the
 * pipeline; this file only wires it to pengine's own device/render
 * pass/command pool/queue and per-frame image count.
 *
 * Call pe_text_init() once after pe_vk_init() (it needs vk_device and
 * friends, and pe_vk_render_pass to exist already). Call pe_text_sync()
 * once per frame before the render pass begins - pe_vk_draw_frame() already
 * does this, so an application normally never has to. Then, from inside
 * pe_vk_draw_scene (or anything else recording into that render pass), wrap
 * drawing with pe_text_begin()/pe_text_end() and call pe_text_draw() any
 * number of times in between. */

int pe_text_init(const char* font_path, float pixel_height);

void pe_text_shutdown();

void pe_text_sync();

void pe_text_begin(VkCommandBuffer cmd, PRenderTarget* target,
                   uint32_t image_index);
void pe_text_end();

/* color components are 0..1. Returns the pen x position after the last
 * glyph. Does nothing outside a pe_text_begin()/pe_text_end() pair. */
float pe_text_draw(const char* utf8_text, vec3 color, float x, float y);

float pe_text_ascent();
float pe_text_cell_width();
float pe_text_cell_height();

#endif
