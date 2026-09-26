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

//about 3 MB, so it belongs in static or heap memory and not on a stack
typedef struct PTerrainTile {
  int tile_x;
  int tile_y;

  u32 texture_count;
  char textures[PE_TERRAIN_TEXTURES_MAX][PE_TERRAIN_TEXTURE_PATH_MAX];

  PTerrainChunk chunks[PE_TERRAIN_CHUNKS];
  PTerrainChunkWater water[PE_TERRAIN_CHUNKS];
} PTerrainTile;

#define PE_TERRAIN_STEPS_PER_TILE (PE_TERRAIN_CHUNKS_PER_SIDE * 8)

//INFO one multiply from the tile index, not the tile origin minus a chunk
//offset minus a step. a border shared by two tiles then comes out bit for bit
//the same on both sides and no crack opens between them. rows run toward -X
//and columns toward -Y, and both count steps from the tile's near corner
static inline float pe_terrain_point_x(const PTerrainTile *tile, float row) {
  return (PE_TERRAIN_MAP_CENTRE_TILE - tile->tile_y -
          row / PE_TERRAIN_STEPS_PER_TILE) *
         PE_TERRAIN_TILE_SIZE;
}

static inline float pe_terrain_point_y(const PTerrainTile *tile,
                                       float column) {
  return (PE_TERRAIN_MAP_CENTRE_TILE - tile->tile_x -
          column / PE_TERRAIN_STEPS_PER_TILE) *
         PE_TERRAIN_TILE_SIZE;
}

//reads base_path.wot and base_path.whm, the Wowee open terrain format, and
//base_path.wwt, the tile's water, if there is one. a tile without water simply
//has no .wwt. tile is only meaningful when this returns true
bool pe_terrain_load(const char *base_path, PTerrainTile *tile);

#endif // !PE_TERRAIN_H
