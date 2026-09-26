#ifndef PE_TERRAIN_MATERIALS_H
#define PE_TERRAIN_MATERIALS_H

#include "terrain_pipeline.h"
#include "terrain_textures.h"

//what a tile's chunks are textured with. the textures are the world's, shared
//by every tile that names them, and only pointed at from here; the alpha maps
//of all 256 chunks are one atlas, so a tile costs one image for them and not
//256
typedef struct PTerrainMaterials {
  VkDescriptorPool pool;
  const PTexture *textures[PE_TERRAIN_TEXTURES_MAX];
  const PTexture *missing;
  PTexture alpha_atlas;

  //the descriptor set to bind for each chunk, set 1 of the terrain pipeline
  VkDescriptorSet chunk_sets[PE_TERRAIN_CHUNKS];
} PTerrainMaterials;

//texture_directory holds the tile's textures as PNG files at the names the
//tile lists, so "a/b.png" is read from texture_directory/a/b.png, the first
//time any tile asks for it.
//needs the renderer up, so from the game's init or later
void pe_vk_terrain_materials_create(const PTerrainPipeline *pipeline,
                                    PTerrainTextures *textures,
                                    const PTerrainTile *tile,
                                    const char *texture_directory,
                                    PTerrainMaterials *materials);

#endif // !PE_TERRAIN_MATERIALS_H
