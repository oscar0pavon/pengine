#ifndef PE_TERRAIN_MATERIALS_H
#define PE_TERRAIN_MATERIALS_H

#include "terrain_pipeline.h"

#include <engine/images.h>

//what a tile's chunks are textured with. the textures, one sampler each, are
//shared by every chunk that names them; the alpha maps of all 256 chunks are
//one atlas, so a tile costs one image for them and not 256
typedef struct PTerrainMaterials {
  VkDescriptorPool pool;
  PTexture fallback;
  PTexture textures[PE_TERRAIN_TEXTURES_MAX];
  PTexture alpha_atlas;

  //the descriptor set to bind for each chunk, set 1 of the terrain pipeline
  VkDescriptorSet chunk_sets[PE_TERRAIN_CHUNKS];
} PTerrainMaterials;

//texture_directory holds the tile's textures as PNG files at the names the
//tile lists, so "a/b.png" is read from texture_directory/a/b.png. one that
//cannot be read is drawn as a magenta checker and logged.
//needs the renderer up, so from the game's init or later
void pe_vk_terrain_materials_create(const PTerrainPipeline *pipeline,
                                    const PTerrainTile *tile,
                                    const char *texture_directory,
                                    PTerrainMaterials *materials);

#endif // !PE_TERRAIN_MATERIALS_H
