#include "terrain_water.h"

#include <stdlib.h>

#define STEPS_PER_CHUNK 8
#define QUAD_INDICES 6

static u32 count_water_chunks(const PTerrainTile *tile) {
  u32 count = 0;
  for (int i = 0; i < PE_TERRAIN_CHUNKS; i++)
    count += tile->water[i].present;
  return count;
}

static u32 count_visible_quads(const PTerrainTile *tile) {
  u32 count = 0;
  for (int i = 0; i < PE_TERRAIN_CHUNKS; i++)
    if (tile->water[i].present)
      for (int quad = 0; quad < PE_TERRAIN_WATER_QUADS; quad++)
        count += tile->water[i].visible[quad] != 0;
  return count;
}

//the grid of corner points sits on the same points as the ground's corner
//vertices, so the two agree wherever the water is as high as the ground
static void build_chunk_vertices(const PTerrainTile *tile, int chunk,
                                 PTerrainWaterVertex *vertices) {
  const PTerrainChunkWater *water = &tile->water[chunk];
  int chunk_row = chunk / PE_TERRAIN_CHUNKS_PER_SIDE;
  int chunk_column = chunk % PE_TERRAIN_CHUNKS_PER_SIDE;

  for (int row = 0; row < PE_TERRAIN_WATER_GRID; row++) {
    for (int column = 0; column < PE_TERRAIN_WATER_GRID; column++) {
      int i = row * PE_TERRAIN_WATER_GRID + column;
      PTerrainWaterVertex *vertex = &vertices[i];

      vertex->position[0] =
          pe_terrain_point_x(tile, chunk_row * STEPS_PER_CHUNK + row);
      vertex->position[1] =
          pe_terrain_point_y(tile, chunk_column * STEPS_PER_CHUNK + column);
      vertex->position[2] = water->heights[i];
      vertex->depth = water->depths[i] / 255.0f;
      vertex->type = (float)water->type;
    }
  }
}

static u32 build_chunk_indices(const PTerrainChunkWater *water,
                               u32 first_vertex, u32 *indices) {
  u32 count = 0;

  for (int row = 0; row < STEPS_PER_CHUNK; row++) {
    for (int column = 0; column < STEPS_PER_CHUNK; column++) {
      if (water->visible[row * STEPS_PER_CHUNK + column] == 0)
        continue;

      u32 near_left = first_vertex + row * PE_TERRAIN_WATER_GRID + column;
      u32 near_right = near_left + 1;
      u32 far_left = near_left + PE_TERRAIN_WATER_GRID;
      u32 far_right = far_left + 1;

      u32 quad[QUAD_INDICES] = {near_left, far_left,  near_right,
                                near_right, far_left, far_right};
      for (int i = 0; i < QUAD_INDICES; i++)
        indices[count++] = quad[i];
    }
  }
  return count;
}

void pe_vk_terrain_water_upload(const PTerrainTile *tile,
                                PTerrainGpuWater *water) {
  water->index_count = 0;

  u32 quads = count_visible_quads(tile);
  if (quads == 0)
    return;

  u32 vertex_count = count_water_chunks(tile) * PE_TERRAIN_WATER_VERTICES;
  PTerrainWaterVertex *vertices = malloc(vertex_count * sizeof(*vertices));
  u32 *indices = malloc(quads * QUAD_INDICES * sizeof(*indices));

  u32 next_vertex = 0;
  u32 next_index = 0;

  for (int chunk = 0; chunk < PE_TERRAIN_CHUNKS; chunk++) {
    if (tile->water[chunk].present == false)
      continue;

    build_chunk_vertices(tile, chunk, &vertices[next_vertex]);
    next_index += build_chunk_indices(&tile->water[chunk], next_vertex,
                                      &indices[next_index]);
    next_vertex += PE_TERRAIN_WATER_VERTICES;
  }

  water->vertex_buffer = pe_vk_create_buffer(
      next_vertex * sizeof(*vertices), vertices,
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
  water->index_buffer = pe_vk_create_buffer(
      next_index * sizeof(*indices), indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
  water->index_count = next_index;

  free(vertices);
  free(indices);
}

void pe_vk_terrain_water_draw(const PTerrainPipeline *pipeline,
                              const PTerrainFrames *frames,
                              const PTerrainGpuWater *water,
                              VkCommandBuffer command, u32 image_index) {
  if (water->index_count == 0)
    return;

  VkDeviceSize no_offset = 0;

  vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipeline->water.pipeline);
  vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipeline->layout, 0, 1, &frames->sets[image_index], 0,
                          NULL);
  vkCmdBindVertexBuffers(command, 0, 1, &water->vertex_buffer.buffer,
                         &no_offset);
  vkCmdBindIndexBuffer(command, water->index_buffer.buffer, 0,
                       VK_INDEX_TYPE_UINT32);
  vkCmdDrawIndexed(command, water->index_count, 1, 0, 0, 0);
}
