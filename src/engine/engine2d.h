#ifndef ENGINE_2D_H
#define ENGINE_2D_H

#include <engine/model.h>

#include <cglm/cglm.h>

typedef struct UV{
  float u;
  float v;
}UV;

void pe_2d_init();
void pe_2d_create_quad_geometry(PModel* model);

void pe_2d_draw(PModel* model, u32 image_index, vec2 position, vec2 size);

void pe_2d_get_character_uvs(UV*out_uvs, char character, float character_pixel_size, float texture_size);

void pe_2d_init_vulkan_buffers(PModel* model);


void pe_2d_create_text_geometry(PModel *model, const char *text, u8 size);

extern mat4 orthogonal_projection;

#endif
