#include "text.h"

#include <pfonts/pfonts.h>
#include <pfonts/vulkan.h>

#include "renderer/commands.h"
#include "renderer/vulkan.h"

static int pe_text_ready = 0;

int pe_text_init(const char* font_path, float pixel_height) {
  if (!pfonts_load_font(font_path, pixel_height)) {
    return 0;
  }

  PFontsVulkanContext context = {0};
  context.device = vk_device;
  context.physical_device = vk_physical_device;
  context.render_pass = pe_vk_render_pass;
  context.command_pool = pe_vk_commands_pool;
  context.queue = vk_queue;
  context.frame_count = main_render_target.images_count;
  context.sample_count = pe_vk_msaa_samples;

  if (!pfonts_vulkan_init(context))
    return 0;

  pe_text_ready = 1;

  return 1;
}

void pe_text_shutdown() {
  if (!pe_text_ready)
    return;

  pfonts_vulkan_shutdown();

  pe_text_ready = 0;
}

/* pe_vk_draw_frame() calls this once per frame, right after starting the
 * frame's command buffer and before pe_vk_start_render_pass() opens the
 * render pass - pfonts_vulkan_sync_atlas() records and submits its own
 * one-shot transfer, which Vulkan does not allow inside an active render
 * pass instance. See vulkan.h in pfonts for why. */
void pe_text_sync() {
  if (!pe_text_ready)
    return;

  pfonts_vulkan_sync_atlas();
}

void pe_text_begin(VkCommandBuffer cmd, PRenderTarget* target,
                   uint32_t image_index) {
  if (!pe_text_ready)
    return;

  pfonts_vulkan_set_extent((float)target->extent.width,
                           (float)target->extent.height);
  pfonts_vulkan_begin_text(cmd, image_index);
}

void pe_text_end() {
  if (!pe_text_ready)
    return;

  pfonts_vulkan_end_text();
}

float pe_text_draw(const char* utf8_text, vec3 color, float x, float y) {
  if (!pe_text_ready)
    return x;

  PColor pfonts_color = {color[0], color[1], color[2]};

  return pfonts_vulkan_draw_text(utf8_text, pfonts_color, x, y);
}

float pe_text_ascent() { return pfonts_get_ascent(); }

float pe_text_cell_width() { return pfonts_get_cell_width(); }

float pe_text_cell_height() { return pfonts_get_cell_height(); }
