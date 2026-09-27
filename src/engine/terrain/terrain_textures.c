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

static PTexture *find(PTerrainTextures *textures, const char *name) {
  for (u32 i = 0; i < textures->count; i++)
    if (strcmp(textures->paths[i], name) == 0)
      return &textures->textures[i];
  return NULL;
}

//a slot for a texture, the next free one or else the one that nothing uses and
//that has been unused for longest, whose image is given back. the gpu is not
//waited for: what drew it was freed before, when the gpu was
static bool take_slot(PTerrainTextures *textures, u32 *slot) {
  if (textures->count < PE_TERRAIN_TEXTURE_CACHE_MAX) {
    *slot = textures->count++;
    return true;
  }

  bool found = false;
  for (u32 i = 0; i < textures->count; i++) {
    if (textures->references[i] > 0)
      continue;
    if (found == false ||
        textures->released_at[i] < textures->released_at[*slot]) {
      *slot = i;
      found = true;
    }
  }

  if (found)
    pe_vk_clean_image(&textures->textures[*slot]);
  return found;
}

const PTexture *pe_vk_terrain_texture_get(PTerrainTextures *textures,
                                          const char *directory,
                                          const char *name) {
  PTexture *loaded = find(textures, name);
  if (loaded) {
    textures->references[loaded - textures->textures]++;
    return loaded;
  }

  char path[PATH_MAX];
  PImage image;
  ZERO(image);
  snprintf(path, sizeof(path), "%s/%s", directory, name);

  if (pe_load_image(path, &image) == -1) {
    LOG("terrain: can't load texture %s\n", path);
    return pe_vk_terrain_texture_missing(textures);
  }

  u32 slot;
  if (take_slot(textures, &slot) == false) {
    LOG("terrain: no room for texture %s, all %d in the cache are in use\n",
        name, PE_TERRAIN_TEXTURE_CACHE_MAX);
    free_image(&image);
    return pe_vk_terrain_texture_missing(textures);
  }

  strcpy(textures->paths[slot], name);
  textures->references[slot] = 1;
  pe_vk_create_texture_from_image(&textures->textures[slot], &image);
  free_image(&image);

  return &textures->textures[slot];
}

void pe_vk_terrain_texture_release(PTerrainTextures *textures,
                                   const PTexture *texture) {
  if (texture < textures->textures ||
      texture >= textures->textures + textures->count)
    return;

  u32 slot = texture - textures->textures;
  if (--textures->references[slot] == 0)
    textures->released_at[slot] = ++textures->releases;
}
