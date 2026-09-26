#include "terrain_textures.h"

#include <engine/log.h>
#include <engine/macros.h>
#include <engine/renderer/vk_images.h>

#include <limits.h>
#include <stdio.h>
#include <string.h>

const PTexture *pe_vk_terrain_texture_missing(PTerrainTextures *textures) {
  if (textures->missing_loaded == false) {
    u8 pixels[2 * 2 * 4] = {255, 0, 255, 255, 0,   0, 0,   255,
                            0,   0, 0,   255, 255, 0, 255, 255};
    PImage image = {.width = 2, .heigth = 2, .pixels_data = pixels};

    pe_vk_create_texture_from_image(&textures->missing, &image);
    textures->missing_loaded = true;
  }
  return &textures->missing;
}

static const PTexture *find(const PTerrainTextures *textures,
                            const char *name) {
  for (u32 i = 0; i < textures->count; i++)
    if (strcmp(textures->paths[i], name) == 0)
      return &textures->textures[i];
  return NULL;
}

const PTexture *pe_vk_terrain_texture_get(PTerrainTextures *textures,
                                          const char *directory,
                                          const char *name) {
  const PTexture *loaded = find(textures, name);
  if (loaded)
    return loaded;

  if (textures->count == PE_TERRAIN_TEXTURE_CACHE_MAX) {
    LOG("terrain: no room for texture %s, the cache holds %d\n", name,
        PE_TERRAIN_TEXTURE_CACHE_MAX);
    return pe_vk_terrain_texture_missing(textures);
  }

  char path[PATH_MAX];
  PImage image;
  ZERO(image);
  snprintf(path, sizeof(path), "%s/%s", directory, name);

  if (pe_load_image(path, &image) == -1) {
    LOG("terrain: can't load texture %s\n", path);
    return pe_vk_terrain_texture_missing(textures);
  }

  u32 slot = textures->count++;
  strcpy(textures->paths[slot], name);
  pe_vk_create_texture_from_image(&textures->textures[slot], &image);
  free_image(&image);

  return &textures->textures[slot];
}
