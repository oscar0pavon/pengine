#ifndef PE_TERRAIN_HEIGHT_H
#define PE_TERRAIN_HEIGHT_H

#include "terrain.h"

//what is needed to ask a tile how high its ground is, kept when the tile
//itself is thrown away after it is uploaded: the height of each of a chunk's
//145 vertices, and its holes
typedef struct PTerrainHeights {
  float heights[PE_TERRAIN_CHUNKS][PE_TERRAIN_CHUNK_VERTICES];
  u16 holes[PE_TERRAIN_CHUNKS];
} PTerrainHeights;

void pe_terrain_heights_from_tile(const PTerrainTile *tile,
                                  PTerrainHeights *heights);

//the height of the surface that is drawn, at a point given in steps from the
//tile's near corner, rows and then columns, each from 0 up to but not
//including 128. false where there is no ground: outside the tile, or in a
//hole
bool pe_terrain_heights_at(const PTerrainHeights *heights, float tile_row,
                           float tile_column, float *height);

#endif // !PE_TERRAIN_HEIGHT_H
