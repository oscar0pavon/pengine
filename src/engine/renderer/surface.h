#ifndef PE_VK_SURFACE_H
#define PE_VK_SURFACE_H

#include "render_target.h"
#include "engine/images.h"

void pe_vk_create_color_resources(PRenderTarget* target);

void pe_vk_set_viewport_and_sccisor(PRenderTarget* target);

void pe_vk_create_surface(PRenderTarget* target, u32 display_index);
void pe_vk_create_display_surface(PRenderTarget* target, u32 display_index);

#endif
