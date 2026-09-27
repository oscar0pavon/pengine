#ifndef PE_TERRAIN_TEXTURES_H
#define PE_TERRAIN_TEXTURES_H

#include "terrain.h"

#include <engine/images.h>

//the ground of the tiles and the buildings on them share it
#define PE_TERRAIN_TEXTURE_CACHE_MAX 2048

//one copy of each texture for the whole world. neighbouring tiles are painted
//from the same tileset, so tiles that each owned their textures would upload
//the same few images once per tile.
//a texture is kept after the last tile and building that used it is gone, in
//case the next one wants it, and is given back only to make room for another
typedef struct PTerrainTextures {
  //the slots in use, one past the last
  u32 count;
  char paths[PE_TERRAIN_TEXTURE_CACHE_MAX][PE_TERRAIN_TEXTURE_PATH_MAX];
  PTexture textures[PE_TERRAIN_TEXTURE_CACHE_MAX];

  //how many tiles and buildings use each, and when the last of them let go of
  //it, counted in releases, so that the one unused for longest is the one given
  //back
  u32 references[PE_TERRAIN_TEXTURE_CACHE_MAX];
  u32 released_at[PE_TERRAIN_TEXTURE_CACHE_MAX];
  u32 releases;

  //what a name that cannot be read is drawn with, made on first use
  PTexture missing;
  bool missing_loaded;
} PTerrainTextures;

//the magenta checker a texture that cannot be read is drawn with
const PTexture *pe_vk_terrain_texture_missing(PTerrainTextures *textures);

//the texture at directory/name, read from disk when it is not in the cache. a
//name that cannot be read is given the magenta checker, so a missing file shows
//on screen rather than as an empty descriptor. whoever takes one lets it go
//with pe_vk_terrain_texture_release() when it stops using it
const PTexture *pe_vk_terrain_texture_get(PTerrainTextures *textures,
                                          const char *directory,
                                          const char *name);

//what pe_vk_terrain_texture_get() returned, given back. the magenta checker is
//not counted, and passing it here does nothing
void pe_vk_terrain_texture_release(PTerrainTextures *textures,
                                   const PTexture *texture);

#endif // !PE_TERRAIN_TEXTURES_H
