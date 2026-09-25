#ifndef PE_TERRAIN_H
#define PE_TERRAIN_H

#include <engine/numbers.h>
#include <stdbool.h>

#define PE_TERRAIN_TILES_PER_SIDE 64
#define PE_TERRAIN_CHUNKS_PER_SIDE 16
#define PE_TERRAIN_CHUNKS (PE_TERRAIN_CHUNKS_PER_SIDE * PE_TERRAIN_CHUNKS_PER_SIDE)

//9x9 corner points and 8x8 centre points of one chunk
#define PE_TERRAIN_CHUNK_VERTICES 145

//the base texture plus up to three that are blended over it
#define PE_TERRAIN_LAYERS_MAX 4

#define PE_TERRAIN_ALPHA_DIM 64
#define PE_TERRAIN_ALPHA_SIZE (PE_TERRAIN_ALPHA_DIM * PE_TERRAIN_ALPHA_DIM)

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

//about 3 MB, so it belongs in static or heap memory and not on a stack
typedef struct PTerrainTile {
  int tile_x;
  int tile_y;

  u32 texture_count;
  char textures[PE_TERRAIN_TEXTURES_MAX][PE_TERRAIN_TEXTURE_PATH_MAX];

  PTerrainChunk chunks[PE_TERRAIN_CHUNKS];
} PTerrainTile;

//reads base_path.wot and base_path.whm, the Wowee open terrain format.
//tile is only meaningful when this returns true
bool pe_terrain_load(const char *base_path, PTerrainTile *tile);

#endif // !PE_TERRAIN_H
