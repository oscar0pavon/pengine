#ifndef PE_DECAL_H
#define PE_DECAL_H

#include <cglm/cglm.h>
#include <vulkan/vulkan_core.h>

#include "ui.h"

/* Textured triangles in the world that lie over what is already drawn: a ring
 * under a selected creature. They are tested against the depth the scene wrote
 * and write none, and blend like a ui quad does, so call pe_decal_begin() after
 * the scene's models and pe_decal_end() before the ui. Call pe_decal_init()
 * once, after pe_ui_init() (it shares its images). */

typedef struct PDecalVertex {
  vec3 position;
  float u, v;
  vec4 color;
} PDecalVertex;

int pe_decal_init();
void pe_decal_shutdown();

void pe_decal_begin(VkCommandBuffer cmd, uint32_t image_index, const mat4 view_projection);
//count is a multiple of 3
void pe_decal_triangles(PUiImage *image, PUiBlend blend, const PDecalVertex *vertices, uint32_t count);
void pe_decal_end();

#endif
