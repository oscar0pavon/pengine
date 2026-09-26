#ifndef PE_TERRAIN_H
#define PE_TERRAIN_H

#include <engine/numbers.h>
#include <stdbool.h>

#define PE_TERRAIN_TILES_PER_SIDE 64

//tile 32 is the middle of the map, where its coordinates are zero
#define PE_TERRAIN_MAP_CENTRE_TILE 32

//yards on a side. kept exact so the border two tiles share is the same number
#define PE_TERRAIN_TILE_SIZE (1600.0f / 3.0f)
#define PE_TERRAIN_CHUNKS_PER_SIDE 16
#define PE_TERRAIN_CHUNKS (PE_TERRAIN_CHUNKS_PER_SIDE * PE_TERRAIN_CHUNKS_PER_SIDE)

//9x9 corner points and 8x8 centre points of one chunk
#define PE_TERRAIN_CHUNK_VERTICES 145

//the base texture plus up to three that are blended over it
#define PE_TERRAIN_LAYERS_MAX 4

#define PE_TERRAIN_ALPHA_DIM 64
#define PE_TERRAIN_ALPHA_SIZE (PE_TERRAIN_ALPHA_DIM * PE_TERRAIN_ALPHA_DIM)

//a chunk's water is a grid of corner points laid over the same 8x8 quads as
//its ground, with a flag per quad for whether there is any water in it
#define PE_TERRAIN_WATER_GRID 9
#define PE_TERRAIN_WATER_VERTICES (PE_TERRAIN_WATER_GRID * PE_TERRAIN_WATER_GRID)
#define PE_TERRAIN_WATER_QUADS 64

//the buildings a tile places: how many kinds, how many placements in all, and
//how long the path a kind is loaded by can be
#define PE_TERRAIN_BUILDINGS_MAX 64
#define PE_TERRAIN_BUILDING_PATH_MAX 128
#define PE_TERRAIN_PLACEMENTS_MAX 256

#define PE_TERRAIN_TEXTURES_MAX 128
#define PE_TERRAIN_TEXTURE_PATH_MAX 128

typedef struct PTerrainChunk {
  float base_height;
  float heights[PE_TERRAIN_CHUNK_VERTICES];

  //4x4 bitmask of the quads left out of the mesh
  u16 holes;

  u32 layer_count;
  u32 layer_textures[PE_TERRAIN_LAYERS_MAX];

  //layer_alpha[i] is how much layer i + 1 covers what is under it, 0 to 255.
  //the base layer has none
  u8 layer_alpha[PE_TERRAIN_LAYERS_MAX - 1][PE_TERRAIN_ALPHA_SIZE];
} PTerrainChunk;

typedef enum PTerrainLiquid {
  PE_TERRAIN_LIQUID_WATER,
  PE_TERRAIN_LIQUID_OCEAN,
  PE_TERRAIN_LIQUID_MAGMA,
  PE_TERRAIN_LIQUID_SLIME
} PTerrainLiquid;

typedef struct PTerrainChunkWater {
  bool present;
  u32 type;
  float heights[PE_TERRAIN_WATER_VERTICES];

  //how deep the liquid is at each point, 0 at the shore to 255
  u8 depths[PE_TERRAIN_WATER_VERTICES];

  //1 for a quad that has liquid in it
  u8 visible[PE_TERRAIN_WATER_QUADS];
} PTerrainChunkWater;

//one building standing in the world. the position and the box are in the
//engine's world, but the rotation is the three degrees the game stores, which
//pe_terrain_placement_matrix() knows how to read
typedef struct PTerrainPlacement {
  u32 building;
  u32 unique_id;
  float position[3];
  float rotation[3];

  //the box the game's own tools computed for it, low corner then high, which
  //is what the placement can be checked against
  float bounds[6];
} PTerrainPlacement;

//about 3 MB, so it belongs in static or heap memory and not on a stack
typedef struct PTerrainTile {
  int tile_x;
  int tile_y;

  u32 texture_count;
  char textures[PE_TERRAIN_TEXTURES_MAX][PE_TERRAIN_TEXTURE_PATH_MAX];

  PTerrainChunk chunks[PE_TERRAIN_CHUNKS];
  PTerrainChunkWater water[PE_TERRAIN_CHUNKS];

  //what the buildings are called, as a path from the data directory to a .wwb
  u32 building_count;
  char buildings[PE_TERRAIN_BUILDINGS_MAX][PE_TERRAIN_BUILDING_PATH_MAX];

  u32 placement_count;
  PTerrainPlacement placements[PE_TERRAIN_PLACEMENTS_MAX];
} PTerrainTile;

#define PE_TERRAIN_STEPS_PER_TILE (PE_TERRAIN_CHUNKS_PER_SIDE * 8)

//INFO the world is X north, Y east, Z up. the game stores it as X north, Y
//west, which is right handed, and the engine's camera is left handed, so
//drawn as stored everything comes out as its own mirror image: facing north
//the west would be on the right. terrain has no handedness of its own and
//that goes unseen until something with text or a lopsided plan is put on it.
//Y is negated here, once, and every other place that turns a position into a
//tile or back goes through these two functions or their inverse.
//
//and one multiply from the tile index, not the tile origin minus a chunk
//offset minus a step. a border shared by two tiles then comes out bit for bit
//the same on both sides and no crack opens between them. rows run toward -X
//and columns toward +Y, and both count steps from the tile's near corner
static inline float pe_terrain_point_x(const PTerrainTile *tile, float row) {
  return (PE_TERRAIN_MAP_CENTRE_TILE - tile->tile_y -
          row / PE_TERRAIN_STEPS_PER_TILE) *
         PE_TERRAIN_TILE_SIZE;
}

static inline float pe_terrain_point_y(const PTerrainTile *tile,
                                       float column) {
  return -(PE_TERRAIN_MAP_CENTRE_TILE - tile->tile_x -
           column / PE_TERRAIN_STEPS_PER_TILE) *
         PE_TERRAIN_TILE_SIZE;
}

//reads base_path.wot and base_path.whm, the Wowee open terrain format, and
//base_path.wwt, the tile's water, if there is one. a tile without water simply
//has no .wwt. tile is only meaningful when this returns true
bool pe_terrain_load(const char *base_path, PTerrainTile *tile);

//a position the way the game's map files store a placement, into the world.
//that space is measured from the far corner of the map, with height second and
//X and Z the ground: north is the far end of Z and east the near end of X
static inline void pe_terrain_adt_to_world(const float adt[3], float world[3]) {
  const double map_centre =
      PE_TERRAIN_MAP_CENTRE_TILE * (double)PE_TERRAIN_TILE_SIZE;

  world[0] = (float)-(adt[2] - map_centre);
  world[1] = (float)(adt[0] - map_centre);
  world[2] = adt[1];
}

#endif // !PE_TERRAIN_H
