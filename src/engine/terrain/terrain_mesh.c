#include "terrain_mesh.h"

#include <string.h>

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

//which side of the tile a lattice coordinate is on: -1 before it, 1 past it
static int tile_side(int lattice) {
  return lattice < 0 ? -1 : (lattice > STEPS_PER_TILE ? 1 : 0);
}

static const PTerrainTile *tile_holding(const PTerrainTile *tile,
                                        const PTerrainNeighbours *neighbours,
                                        int row, int column) {
  int row_side = tile_side(row);
  int column_side = tile_side(column);

  if (row_side == 0 && column_side == 0)
    return tile;
  return neighbours ? neighbours->tiles[row_side + 1][column_side + 1] : NULL;
}

//row and column count 0 to 128 across the tile, on the corner points only, and
//may be one past either end when the tile next door is there to answer
static float tile_corner_height(const PTerrainTile *tile,
                                const PTerrainNeighbours *neighbours, int row,
                                int column) {
  const PTerrainTile *holder = tile_holding(tile, neighbours, row, column);
  row -= tile_side(row) * STEPS_PER_TILE;
  column -= tile_side(column) * STEPS_PER_TILE;

  int chunk_row = clamp_int(row / STEPS_PER_CHUNK, 0,
                            PE_TERRAIN_CHUNKS_PER_SIDE - 1);
  int chunk_column = clamp_int(column / STEPS_PER_CHUNK, 0,
                               PE_TERRAIN_CHUNKS_PER_SIDE - 1);
  const PTerrainChunk *chunk =
      &holder->chunks[chunk_row * PE_TERRAIN_CHUNKS_PER_SIDE + chunk_column];

  return chunk_height(chunk, row - chunk_row * STEPS_PER_CHUNK,
                      column - chunk_column * STEPS_PER_CHUNK);
}

static float corner_slope(float low, float high, int span) {
  return (high - low) / span;
}

//INFO the slope comes from the corner points alone, read across chunk borders
//and, when the tile next door is loaded, tile borders, so a vertex on a border
//gets the same normal from either side that owns it. the centre vertices are
//left out on purpose: the map artist places them freely, and shading through
//them turns every quad into a visible star
static void surface_normal(const PTerrainTile *tile,
                           const PTerrainNeighbours *neighbours,
                           int lattice_row, int lattice_column,
                           vec3 normal) {
  float row_slope;
  float column_slope;

  if (lattice_row % 2 == 0) {
    int row = lattice_row / 2;
    int column = lattice_column / 2;

    int row_before = tile_holding(tile, neighbours, row - 1, column)
                         ? row - 1 : row;
    int row_after = tile_holding(tile, neighbours, row + 1, column)
                        ? row + 1 : row;
    int column_before = tile_holding(tile, neighbours, row, column - 1)
                            ? column - 1 : column;
    int column_after = tile_holding(tile, neighbours, row, column + 1)
                           ? column + 1 : column;

    row_slope = corner_slope(
        tile_corner_height(tile, neighbours, row_before, column),
        tile_corner_height(tile, neighbours, row_after, column),
        row_after - row_before);
    column_slope = corner_slope(
        tile_corner_height(tile, neighbours, row, column_before),
        tile_corner_height(tile, neighbours, row, column_after),
        column_after - column_before);
  } else {
    int row = lattice_row / 2;
    int column = lattice_column / 2;
    float top_left = tile_corner_height(tile, neighbours, row, column);
    float top_right = tile_corner_height(tile, neighbours, row, column + 1);
    float bottom_left = tile_corner_height(tile, neighbours, row + 1, column);
    float bottom_right =
        tile_corner_height(tile, neighbours, row + 1, column + 1);

    row_slope = ((bottom_left + bottom_right) - (top_left + top_right)) / 2;
    column_slope = ((top_right + bottom_right) - (top_left + bottom_left)) / 2;
  }

  //rows run toward -X and columns toward +Y, so a slope up along a row is a
  //surface that faces +X, and a slope up along a column faces -Y
  glm_vec3_copy((vec3){row_slope / STEP_SIZE, -column_slope / STEP_SIZE, 1},
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

//INFO a border two tiles share is stored in both, as base plus height in each,
//and the two sums do not always round to the same float. one bit is enough to
//open a crack a pixel wide along the border. so the line belongs to the tile on
//its low side, and this tile reads its far row and column from that neighbour
//whenever it is loaded
static float vertex_height(const PTerrainTile *tile,
                           const PTerrainNeighbours *neighbours,
                           const PTerrainChunk *chunk, int index,
                           float tile_row, float tile_column) {
  int row = (int)tile_row;
  int column = (int)tile_column;
  bool on_corner_point = row == tile_row && column == tile_column;

  int side_row = on_corner_point && row == STEPS_PER_TILE ? 1 : 0;
  int side_column = on_corner_point && column == STEPS_PER_TILE ? 1 : 0;

  const PTerrainTile *owner = NULL;
  if (neighbours && (side_row || side_column))
    owner = neighbours->tiles[side_row + 1][side_column + 1];

  if (owner == NULL)
    return chunk->base_height + chunk->heights[index];

  return tile_corner_height(owner, NULL, row - side_row * STEPS_PER_TILE,
                            column - side_column * STEPS_PER_TILE);
}

static void build_chunk_vertices(const PTerrainTile *tile,
                                 const PTerrainNeighbours *neighbours,
                                 int chunk_row, int chunk_column,
                                 PTerrainVertex *vertices) {
  const PTerrainChunk *chunk =
      &tile->chunks[chunk_row * PE_TERRAIN_CHUNKS_PER_SIDE + chunk_column];

  for (int i = 0; i < PE_TERRAIN_CHUNK_VERTICES; i++) {
    PTerrainVertex *vertex = &vertices[i];
    float row;
    float column;
    grid_offset(i, &row, &column);

    float tile_row = chunk_row * STEPS_PER_CHUNK + row;
    float tile_column = chunk_column * STEPS_PER_CHUNK + column;

    vertex->position[0] = pe_terrain_point_x(tile, tile_row);
    vertex->position[1] = pe_terrain_point_y(tile, tile_column);
    vertex->position[2] =
        vertex_height(tile, neighbours, chunk, i, tile_row, tile_column);

    surface_normal(tile, neighbours, (int)(tile_row * 2 + 0.5f),
                   (int)(tile_column * 2 + 0.5f), vertex->normal);

    vertex->uv[0] = vertex->position[1] * TEXTURE_SCALE;
    vertex->uv[1] = -vertex->position[0] * TEXTURE_SCALE;

    vertex->layer_uv[0] =
        (column * ALPHA_TEXELS_PER_QUAD + 0.5f) / PE_TERRAIN_ALPHA_DIM;
    vertex->layer_uv[1] =
        (row * ALPHA_TEXELS_PER_QUAD + 0.5f) / PE_TERRAIN_ALPHA_DIM;
  }
}

static void build_chunk_bounds(const PTerrainVertex *vertices, vec4 bounds) {
  vec3 low;
  vec3 high;
  glm_vec3_copy((float *)vertices[0].position, low);
  glm_vec3_copy((float *)vertices[0].position, high);

  for (int i = 1; i < PE_TERRAIN_CHUNK_VERTICES; i++) {
    glm_vec3_minv(low, (float *)vertices[i].position, low);
    glm_vec3_maxv(high, (float *)vertices[i].position, high);
  }

  glm_vec3_center(low, high, bounds);
  bounds[3] = glm_vec3_distance(low, high) / 2;
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

void pe_terrain_mesh_build(const PTerrainTile *tile,
                           const PTerrainNeighbours *neighbours,
                           PTerrainMesh *mesh) {
  mesh->index_count = 0;

  for (int chunk_row = 0; chunk_row < PE_TERRAIN_CHUNKS_PER_SIDE; chunk_row++) {
    for (int chunk_column = 0; chunk_column < PE_TERRAIN_CHUNKS_PER_SIDE;
         chunk_column++) {
      int chunk_index = chunk_row * PE_TERRAIN_CHUNKS_PER_SIDE + chunk_column;
      u32 first_vertex = chunk_index * PE_TERRAIN_CHUNK_VERTICES;
      PTerrainChunkRange *range = &mesh->chunks[chunk_index];

      build_chunk_vertices(tile, neighbours, chunk_row, chunk_column,
                           &mesh->vertices[first_vertex]);
      build_chunk_bounds(&mesh->vertices[first_vertex],
                         mesh->bounds[chunk_index]);

      range->first_index = mesh->index_count;
      range->index_count = build_chunk_indices(
          &tile->chunks[chunk_index], first_vertex,
          &mesh->indices[mesh->index_count]);
      mesh->index_count += range->index_count;
    }
  }
}
