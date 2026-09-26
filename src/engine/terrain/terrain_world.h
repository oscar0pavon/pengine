#ifndef PE_TERRAIN_WORLD_H
#define PE_TERRAIN_WORLD_H

#include "terrain_buildings.h"
#include "terrain_draw.h"
#include "terrain_height.h"
#include "terrain_water.h"

//how many tiles across a loaded block can be, five by five
#define PE_TERRAIN_WORLD_SIDE_MAX 5
#define PE_TERRAIN_WORLD_TILES_MAX                                             \
  (PE_TERRAIN_WORLD_SIDE_MAX * PE_TERRAIN_WORLD_SIDE_MAX)

typedef struct PTerrainWorldTile {
  int tile_x;
  int tile_y;
  PTerrainGpuMesh mesh;
  PTerrainMaterials materials;
  PTerrainGpuWater water;
  PTerrainHeights heights;
} PTerrainWorldTile;

//everything terrain needs on the gpu: the pipeline and the frame's uniforms
//are made once, the textures are shared, and each tile has its own mesh and
//materials
typedef struct PTerrainWorld {
  PTerrainPipeline pipeline;
  PTerrainFrames frames;
  PTerrainTextures textures;
  PTerrainBuildings buildings;

  //how many buildings the last draw recorded, for whoever wants to know
  u32 buildings_drawn;

  u32 tile_count;
  PTerrainWorldTile tiles[PE_TERRAIN_WORLD_TILES_MAX];
} PTerrainWorld;

//needs the renderer up, so from the game's init or later
void pe_vk_terrain_world_create(PTerrainWorld *world);

//loads the square of tiles that reaches radius tiles from the centre one, from
//directory/map_x_y.wot and .whm, textures from the same directory. a tile
//whose files are not there is left out, and its neighbours are lit as if it
//were the edge of the world. false if there was no tile at all.
//tiles stay loaded: there is nothing here yet that gives a tile's gpu memory
//back
bool pe_vk_terrain_world_load_area(PTerrainWorld *world, const char *directory,
                                   const char *map, int centre_x, int centre_y,
                                   int radius);

//the height of the ground at a world position, the surface that is drawn and
//not a flat guess from its corners. false where there is none: no loaded tile
//there, or a hole in it, which is a building or a cave where the ground is
//left out. this is the ground only, and water is not looked at
bool pe_terrain_world_height_at(const PTerrainWorld *world, float x, float y,
                                float *height);

//sends the frame to the gpu, then records the sky, the ground of every tile,
//the buildings on it, and last the water of every tile, which blends over both. call it from
//the pe_vk_draw_scene hook. returns how many chunks of ground were drawn
u32 pe_vk_terrain_world_draw(PTerrainWorld *world, const PTerrainFrame *frame,
                             VkCommandBuffer command, u32 image_index);

#endif // !PE_TERRAIN_WORLD_H
