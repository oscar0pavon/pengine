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

  //how many buildings and props the last draw recorded, for whoever wants to
  //know
  u32 buildings_drawn;

  u32 tile_count;
  PTerrainWorldTile tiles[PE_TERRAIN_WORLD_TILES_MAX];

  //the tiles the game has none of, or that could not be read, so that they are
  //not looked for again every frame. indexed by tile_y and tile_x
  bool unavailable[PE_TERRAIN_TILES_PER_SIDE][PE_TERRAIN_TILES_PER_SIDE];
} PTerrainWorld;

//needs the renderer up, so from the game's init or later
void pe_vk_terrain_world_create(PTerrainWorld *world);

//how far past the streaming distance a loaded tile is kept, so that a camera
//crossing a border back and forth does not load and unload the tiles there
//over and over. the distance and this together should stay under two tiles, or
//more than PE_TERRAIN_WORLD_SIDE_MAX tiles across can be wanted
#define PE_TERRAIN_STREAM_MARGIN 100.0f

//keeps the tiles within distance yards of a world position loaded, from
//directory/map_x_y.wot and .whm, textures from the same directory, and gives
//back the ground, buildings and props of those farther than distance and the
//margin. a tile whose files are not there is left out, and its neighbours are
//lit as if it were the edge of the world.
//it loads one tile at most a call, the nearest first, so the caller can spread
//the work over frames by calling it every one, or fill the world by calling it
//until it returns false. it returns whether it loaded or unloaded anything.
//the gpu is waited for when a tile is unloaded, so that is where a frame is
//lost. the textures of a tile stay in the world's cache after it is gone
bool pe_vk_terrain_world_stream(PTerrainWorld *world, const char *directory,
                                const char *map, float x, float y,
                                float distance);

//the height of the ground at a world position, the surface that is drawn and
//not a flat guess from its corners. false where there is none: no loaded tile
//there, or a hole in it, which is a building or a cave where the ground is
//left out. this is the ground only, and water is not looked at
bool pe_terrain_world_height_at(const PTerrainWorld *world, float x, float y,
                                float *height);

//the height of the floor a walker stands on at x and y, when it is at z: the
//ground, or the highest floor of a building or a prop at or below z, whichever
//is higher.
//a roof or an upper floor over its head is not looked at. false where there is
//neither
bool pe_terrain_world_floor_at(const PTerrainWorld *world, float x, float y,
                               float z, float *height);

//moves the centre of a sphere out of the walls of the buildings and props, and
//returns whether it moved. this is for a walker's body, and it leaves the ground and the
//floors to pe_terrain_world_floor_at()
bool pe_terrain_world_push_out(const PTerrainWorld *world, vec3 centre,
                               float radius);

//sends the frame to the gpu, then records the sky, the ground of every tile,
//the buildings and props on it, and last the water of every tile, which blends
//over both. call it from the pe_vk_draw_scene hook. returns how many chunks of
//ground were drawn
u32 pe_vk_terrain_world_draw(PTerrainWorld *world, const PTerrainFrame *frame,
                             VkCommandBuffer command, u32 image_index);

#endif // !PE_TERRAIN_WORLD_H
