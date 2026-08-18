#ifndef PE_VK_SURFACE_H
#define PE_VK_SURFACE_H

#include "engine/images.h"

void pe_vk_create_surface(void);

void pe_vk_create_color_resources(void);

void pe_vk_set_viewport_and_sccisor(void);

extern PTexture vk_color_image;

#endif
