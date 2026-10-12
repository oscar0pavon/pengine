#include "terrain_height.h"

#define STEPS_PER_CHUNK 8
#define ROW_WIDTH 17
#define CENTRES_OFFSET 9

void pe_terrain_heights_from_tile(const PTerrainTile *tile,
                                  PTerrainHeights *heights) {
  for (int chunk = 0; chunk < PE_TERRAIN_CHUNKS; chunk++) {
    const PTerrainChunk *source = &tile->chunks[chunk];

    for (int i = 0; i < PE_TERRAIN_CHUNK_VERTICES; i++)
      heights->heights[chunk][i] = source->base_height + source->heights[i];
    heights->holes[chunk] = source->holes;
  }
}

static bool quad_is_hole(u16 holes, int row, int column) {
  return (holes >> ((row / 2) * 4 + column / 2)) & 1;
}

static int clamp_int(int value, int low, int high) {
  return value < low ? low : (value > high ? high : value);
}

//INFO a quad is drawn as four triangles fanned from its centre vertex, so the
//ground is not the plane of its four corners: the centre sits wherever the map
//artist put it, often a yard off that plane on a slope. u runs along the
//columns and v along the rows, both 0 to 1 across the quad, and the wedge the
//point is in gives its three weights, the third always the centre's
static float fan_height(const float *chunk_heights, int quad_row,
                        int quad_column, float u, float v) {
  int top_left = quad_row * ROW_WIDTH + quad_column;
  float centre = chunk_heights[CENTRES_OFFSET + top_left];

  float first;
  float second;
  float first_weight;
  float second_weight;

  if (u > v) {
    if (u + v < 1) {
      first = chunk_heights[top_left];
      second = chunk_heights[top_left + 1];
      first_weight = 1 - u - v;
      second_weight = u - v;
    } else {
      first = chunk_heights[top_left + 1];
      second = chunk_heights[top_left + ROW_WIDTH + 1];
      first_weight = u - v;
      second_weight = u + v - 1;
    }
  } else {
    if (u + v < 1) {
      first = chunk_heights[top_left];
      second = chunk_heights[top_left + ROW_WIDTH];
      first_weight = 1 - u - v;
      second_weight = v - u;
    } else {
      first = chunk_heights[top_left + ROW_WIDTH];
      second = chunk_heights[top_left + ROW_WIDTH + 1];
      first_weight = v - u;
      second_weight = u + v - 1;
    }
  }

  float centre_weight = 1 - first_weight - second_weight;
  return first * first_weight + second * second_weight +
         centre * centre_weight;
}

bool pe_terrain_heights_at(const PTerrainHeights *heights, float tile_row,
                           float tile_column, float *height) {
  if (tile_row < 0 || tile_column < 0 ||
      tile_row >= PE_TERRAIN_STEPS_PER_TILE ||
      tile_column >= PE_TERRAIN_STEPS_PER_TILE)
    return false;

  int chunk_row = clamp_int((int)(tile_row / STEPS_PER_CHUNK), 0,
                            PE_TERRAIN_CHUNKS_PER_SIDE - 1);
  int chunk_column = clamp_int((int)(tile_column / STEPS_PER_CHUNK), 0,
                               PE_TERRAIN_CHUNKS_PER_SIDE - 1);
  int chunk = chunk_row * PE_TERRAIN_CHUNKS_PER_SIDE + chunk_column;

  float row = tile_row - chunk_row * STEPS_PER_CHUNK;
  float column = tile_column - chunk_column * STEPS_PER_CHUNK;

  int quad_row = clamp_int((int)row, 0, STEPS_PER_CHUNK - 1);
  int quad_column = clamp_int((int)column, 0, STEPS_PER_CHUNK - 1);

  if (quad_is_hole(heights->holes[chunk], quad_row, quad_column))
    return false;

  *height = fan_height(heights->heights[chunk], quad_row, quad_column,
                       column - quad_column, row - quad_row);
  return true;
}
