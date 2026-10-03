#ifndef PE_UI_H
#define PE_UI_H

#include <cglm/cglm.h>
#include <vulkan/vulkan_core.h>

#include "renderer/render_target.h"

/* Textured, alpha blended quads in pixel space (origin top left, y down),
 * drawn over the scene after pe_vk_draw_scene's own models. Quads are drawn in
 * the order they are given, so a caller paints back to front. Call
 * pe_ui_init() once after pe_vk_init(), then wrap the quads of a frame in
 * pe_ui_begin()/pe_ui_end() from inside the pe_vk_draw_scene hook. */

typedef struct PUiImage PUiImage;

typedef enum PUiBlend {
  PE_UI_BLEND_ALPHA,
  PE_UI_BLEND_ADD,
} PUiBlend;

typedef struct PUiQuad {
  PUiImage *image;
  float x, y, width, height;
  float u0, v0, u1, v1;
  vec4 color;
  PUiBlend blend;
  //u runs down the quad and v across it, for art stored turned a quarter
  bool transpose_uv;
} PUiQuad;

int pe_ui_init();
void pe_ui_shutdown();

//a png, loaded once per path and kept until pe_ui_shutdown(). NULL if it can't
//be read
PUiImage *pe_ui_image(const char *path);

//an image that is one white texel, for a quad that is only its colour
PUiImage *pe_ui_image_white();

void pe_ui_image_size(const PUiImage *image, float *width, float *height);

void pe_ui_begin(VkCommandBuffer cmd, PRenderTarget *target,
                 uint32_t image_index);
void pe_ui_quad(const PUiQuad *quad);
void pe_ui_end();

#endif
