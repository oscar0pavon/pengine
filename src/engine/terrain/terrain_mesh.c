#include "terrain_mesh.h"

#include <string.h>

#define WORLD_CENTER_TILE 32
#define STEPS_PER_CHUNK 8
#define STEPS_PER_TILE (PE_TERRAIN_CHUNKS_PER_SIDE * STEPS_PER_CHUNK)
#define STEP_SIZE (PE_TERRAIN_TILE_SIZE / STEPS_PER_TILE)

//a chunk stores its 9 rows as 9 corners then 8 centres, so a row is 17 wide
#define ROW_WIDTH 17
#define CENTRES_OFFSET 9

#define QUAD_INDICES (4 * 3)

//each texture repeats this often across a chunk. a repeat per chunk makes the
//chunk grid readable in the ground, this many does not
#define TEXTURE_REPEATS_PER_CHUNK 4.0f
#define TEXTURE_SCALE                                                          \
  (TEXTURE_REPEATS_PER_CHUNK * PE_TERRAIN_CHUNKS_PER_SIDE /                    \
   PE_TERRAIN_TILE_SIZE)

//63 texel steps across the 8 quads, sampled at texel centres so the edge of
//one chunk's map does not pull in the texel beyond it
#define ALPHA_TEXELS_PER_QUAD ((PE_TERRAIN_ALPHA_DIM - 1) / 8.0f)

static float chunk_height(const PTerrainChunk *chunk, int row, int column) {
  return chunk->base_height + chunk->heights[row * ROW_WIDTH + column];
}

static int clamp_int(int value, int low, int high) {
  return value < low ? low : (value > high ? high : value);
}

//row and column count 0 to 128 across the tile, on the corner points only
static float tile_corner_height(const PTerrainTile *tile, int row, int column) {
  int chunk_row = clamp_int(row / STEPS_PER_CHUNK, 0,
                            PE_TERRAIN_CHUNKS_PER_SIDE - 1);
  int chunk_column = clamp_int(column / STEPS_PER_CHUNK, 0,
                               PE_TERRAIN_CHUNKS_PER_SIDE - 1);
  const PTerrainChunk *chunk =
      &tile->chunks[chunk_row * PE_TERRAIN_CHUNKS_PER_SIDE + chunk_column];

  return chunk_height(chunk, row - chunk_row * STEPS_PER_CHUNK,
                      column - chunk_column * STEPS_PER_CHUNK);
}

static float corner_slope(float low, float high, int span) {
  return (high - low) / span;
}

//INFO the slope comes from the corner points alone, read across chunk borders,
//so a vertex on a border gets the same normal from either chunk that owns it.
//the centre vertices are left out on purpose: the map artist places them
//freely, and shading through them turns every quad into a visible star
static void surface_normal(const PTerrainTile *tile, int lattice_row,
                           int lattice_column, vec3 normal) {
  float row_slope;
  float column_slope;

  if (lattice_row % 2 == 0) {
    int row = lattice_row / 2;
    int column = lattice_column / 2;
    int row_before = clamp_int(row - 1, 0, STEPS_PER_TILE);
    int row_after = clamp_int(row + 1, 0, STEPS_PER_TILE);
    int column_before = clamp_int(column - 1, 0, STEPS_PER_TILE);
    int column_after = clamp_int(column + 1, 0, STEPS_PER_TILE);

    row_slope = corner_slope(tile_corner_height(tile, row_before, column),
                             tile_corner_height(tile, row_after, column),
                             row_after - row_before);
    column_slope = corner_slope(tile_corner_height(tile, row, column_before),
                                tile_corner_height(tile, row, column_after),
                                column_after - column_before);
  } else {
    int row = lattice_row / 2;
    int column = lattice_column / 2;
    float top_left = tile_corner_height(tile, row, column);
    float top_right = tile_corner_height(tile, row, column + 1);
    float bottom_left = tile_corner_height(tile, row + 1, column);
    float bottom_right = tile_corner_height(tile, row + 1, column + 1);

    row_slope = ((bottom_left + bottom_right) - (top_left + top_right)) / 2;
    column_slope = ((top_right + bottom_right) - (top_left + bottom_left)) / 2;
  }

  //rows run toward -X and columns toward -Y, so a slope up along a row is a
  //surface that faces +X
  glm_vec3_copy((vec3){row_slope / STEP_SIZE, column_slope / STEP_SIZE, 1},
                normal);
  glm_vec3_normalize(normal);
}

static void grid_offset(int index, float *row, float *column) {
  int x = index % ROW_WIDTH;
  int y = index / ROW_WIDTH;

  if (x < CENTRES_OFFSET) {
    *row = y;
    *column = x;
  } else {
    *row = y + 0.5f;
    *column = x - (CENTRES_OFFSET - 0.5f);
  }
}

static void build_chunk_vertices(const PTerrainTile *tile, int chunk_row,
                                 int chunk_column, PTerrainVertex *vertices) {
  const PTerrainChunk *chunk =
      &tile->chunks[chunk_row * PE_TERRAIN_CHUNKS_PER_SIDE + chunk_column];

  for (int i = 0; i < PE_TERRAIN_CHUNK_VERTICES; i++) {
    PTerrainVertex *vertex = &vertices[i];
    float row;
    float column;
    grid_offset(i, &row, &column);

    float tile_row = chunk_row * STEPS_PER_CHUNK + row;
    float tile_column = chunk_column * STEPS_PER_CHUNK + column;

    //INFO one multiply from the tile index, not the tile origin minus a chunk
    //offset minus a step. a border shared by two tiles then comes out bit for
    //bit the same on both sides and no crack opens between them
    vertex->position[0] =
        (WORLD_CENTER_TILE - tile->tile_y - tile_row / STEPS_PER_TILE) *
        PE_TERRAIN_TILE_SIZE;
    vertex->position[1] =
        (WORLD_CENTER_TILE - tile->tile_x - tile_column / STEPS_PER_TILE) *
        PE_TERRAIN_TILE_SIZE;
    vertex->position[2] = chunk->base_height + chunk->heights[i];

    surface_normal(tile, (int)(tile_row * 2 + 0.5f),
                   (int)(tile_column * 2 + 0.5f), vertex->normal);

    vertex->uv[0] = -vertex->position[1] * TEXTURE_SCALE;
    vertex->uv[1] = -vertex->position[0] * TEXTURE_SCALE;

    vertex->layer_uv[0] =
        (column * ALPHA_TEXELS_PER_QUAD + 0.5f) / PE_TERRAIN_ALPHA_DIM;
    vertex->layer_uv[1] =
        (row * ALPHA_TEXELS_PER_QUAD + 0.5f) / PE_TERRAIN_ALPHA_DIM;
  }
}

static bool quad_is_hole(u16 holes, int row, int column) {
  return (holes >> ((row / 2) * 4 + column / 2)) & 1;
}

static u32 add_quad(u32 *indices, u32 first_vertex, int row, int column) {
  u32 centre = first_vertex + CENTRES_OFFSET + row * ROW_WIDTH + column;
  u32 top_left = centre - CENTRES_OFFSET;
  u32 top_right = centre - CENTRES_OFFSET + 1;
  u32 bottom_left = centre + CENTRES_OFFSET - 1;
  u32 bottom_right = centre + CENTRES_OFFSET;

  u32 fan[QUAD_INDICES] = {
      centre, top_left,     top_right,   //
      centre, top_right,    bottom_right, //
      centre, bottom_right, bottom_left, //
      centre, bottom_left,  top_left};

  memcpy(indices, fan, sizeof(fan));
  return QUAD_INDICES;
}

static u32 build_chunk_indices(const PTerrainChunk *chunk, u32 first_vertex,
                               u32 *indices) {
  u32 count = 0;

  for (int row = 0; row < STEPS_PER_CHUNK; row++)
    for (int column = 0; column < STEPS_PER_CHUNK; column++)
      if (quad_is_hole(chunk->holes, row, column) == false)
        count += add_quad(indices + count, first_vertex, row, column);

  return count;
}

void pe_terrain_mesh_build(const PTerrainTile *tile, PTerrainMesh *mesh) {
  mesh->index_count = 0;

  for (int chunk_row = 0; chunk_row < PE_TERRAIN_CHUNKS_PER_SIDE; chunk_row++) {
    for (int chunk_column = 0; chunk_column < PE_TERRAIN_CHUNKS_PER_SIDE;
         chunk_column++) {
      int chunk_index = chunk_row * PE_TERRAIN_CHUNKS_PER_SIDE + chunk_column;
      u32 first_vertex = chunk_index * PE_TERRAIN_CHUNK_VERTICES;
      PTerrainChunkRange *range = &mesh->chunks[chunk_index];

      build_chunk_vertices(tile, chunk_row, chunk_column,
                           &mesh->vertices[first_vertex]);

      range->first_index = mesh->index_count;
      range->index_count = build_chunk_indices(
          &tile->chunks[chunk_index], first_vertex,
          &mesh->indices[mesh->index_count]);
      mesh->index_count += range->index_count;
    }
  }
}
