#ifndef PE_TERRAIN_TEXTURES_H
#define PE_TERRAIN_TEXTURES_H

#include "terrain.h"

#include <engine/images.h>

//the ground of the tiles and the buildings on them share it
#define PE_TERRAIN_TEXTURE_CACHE_MAX 512

//one copy of each texture for the whole world. neighbouring tiles are painted
//from the same tileset, so tiles that each owned their textures would upload
//the same few images once per tile
typedef struct PTerrainTextures {
  u32 count;
  char paths[PE_TERRAIN_TEXTURE_CACHE_MAX][PE_TERRAIN_TEXTURE_PATH_MAX];
  PTexture textures[PE_TERRAIN_TEXTURE_CACHE_MAX];

  //what a name that cannot be read is drawn with, made on first use
  PTexture missing;
  bool missing_loaded;
} PTerrainTextures;

//the magenta checker a texture that cannot be read is drawn with
const PTexture *pe_vk_terrain_texture_missing(PTerrainTextures *textures);

//the texture at directory/name, read from disk only the first time it is
//asked for. a name that cannot be read is given the magenta checker, so a
//missing file shows on screen rather than as an empty descriptor
const PTexture *pe_vk_terrain_texture_get(PTerrainTextures *textures,
                                          const char *directory,
                                          const char *name);

#endif // !PE_TERRAIN_TEXTURES_H
