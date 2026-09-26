#include "terrain_world.h"

#include <engine/log.h>

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct LoadedArea {
  int first_x;
  int first_y;
  int side;
  PTerrainTile *tiles[PE_TERRAIN_WORLD_SIDE_MAX][PE_TERRAIN_WORLD_SIDE_MAX];
} LoadedArea;

void pe_vk_terrain_world_create(PTerrainWorld *world) {
  pe_vk_terrain_pipeline_create(&world->pipeline);
  pe_vk_terrain_frames_create(&world->pipeline, &world->frames);
}

static bool tile_files_exist(const char *directory, const char *map,
                             int tile_x, int tile_y) {
  char path[PATH_MAX];

  snprintf(path, sizeof(path), "%s/%s_%d_%d.wot", directory, map, tile_x,
           tile_y);
  if (access(path, R_OK) != 0)
    return false;

  snprintf(path, sizeof(path), "%s/%s_%d_%d.whm", directory, map, tile_x,
           tile_y);
  return access(path, R_OK) == 0;
}

static PTerrainTile *read_tile(const char *directory, const char *map,
                               int tile_x, int tile_y) {
  if (tile_files_exist(directory, map, tile_x, tile_y) == false)
    return NULL;

  char base[PATH_MAX];
  snprintf(base, sizeof(base), "%s/%s_%d_%d", directory, map, tile_x, tile_y);

  PTerrainTile *tile = malloc(sizeof(PTerrainTile));
  if (pe_terrain_load(base, tile) == false) {
    free(tile);
    return NULL;
  }
  return tile;
}

static const PTerrainTile *tile_in_area(const LoadedArea *area, int grid_x,
                                        int grid_y) {
  if (grid_x < 0 || grid_y < 0 || grid_x >= area->side || grid_y >= area->side)
    return NULL;
  return area->tiles[grid_y][grid_x];
}

static void find_neighbours(const LoadedArea *area, int grid_x, int grid_y,
                            PTerrainNeighbours *neighbours) {
  for (int row = 0; row < 3; row++)
    for (int column = 0; column < 3; column++)
      neighbours->tiles[row][column] =
          tile_in_area(area, grid_x + column - 1, grid_y + row - 1);
}

static void upload_tile(PTerrainWorld *world, const PTerrainTile *tile,
                        const PTerrainNeighbours *neighbours,
                        PTerrainMesh *mesh, const char *directory) {
  PTerrainWorldTile *out = &world->tiles[world->tile_count++];
  out->tile_x = tile->tile_x;
  out->tile_y = tile->tile_y;

  pe_terrain_mesh_build(tile, neighbours, mesh);
  pe_vk_terrain_mesh_upload(mesh, &out->mesh);
  pe_vk_terrain_materials_create(&world->pipeline, &world->textures, tile,
                                 directory, &out->materials);
  pe_vk_terrain_water_upload(tile, &out->water);
  pe_terrain_heights_from_tile(tile, &out->heights);
}

bool pe_vk_terrain_world_load_area(PTerrainWorld *world, const char *directory,
                                   const char *map, int centre_x, int centre_y,
                                   int radius) {
  LoadedArea area = {.first_x = centre_x - radius,
                     .first_y = centre_y - radius,
                     .side = 2 * radius + 1};

  if (area.side > PE_TERRAIN_WORLD_SIDE_MAX ||
      world->tile_count + area.side * area.side > PE_TERRAIN_WORLD_TILES_MAX) {
    LOG("terrain: %d by %d tiles do not fit in the world\n", area.side,
        area.side);
    return false;
  }

  for (int y = 0; y < area.side; y++)
    for (int x = 0; x < area.side; x++)
      area.tiles[y][x] =
          read_tile(directory, map, area.first_x + x, area.first_y + y);

  PTerrainMesh *mesh = malloc(sizeof(PTerrainMesh));
  u32 loaded_before = world->tile_count;

  for (int y = 0; y < area.side; y++) {
    for (int x = 0; x < area.side; x++) {
      if (area.tiles[y][x] == NULL)
        continue;

      PTerrainNeighbours neighbours;
      find_neighbours(&area, x, y, &neighbours);
      upload_tile(world, area.tiles[y][x], &neighbours, mesh, directory);
    }
  }

  for (int y = 0; y < area.side; y++)
    for (int x = 0; x < area.side; x++)
      free(area.tiles[y][x]);
  free(mesh);

  return world->tile_count > loaded_before;
}

static const PTerrainWorldTile *find_tile(const PTerrainWorld *world,
                                          int tile_x, int tile_y) {
  for (u32 i = 0; i < world->tile_count; i++)
    if (world->tiles[i].tile_x == tile_x && world->tiles[i].tile_y == tile_y)
      return &world->tiles[i];
  return NULL;
}

//INFO x and y are worked back to tiles in double. the tile size times a few
//dozen tiles is a number in the thousands, and a float has about a thousandth
//of a yard left at that size, which is a tenth of a step of the ground
bool pe_terrain_world_height_at(const PTerrainWorld *world, float x, float y,
                                float *height) {
  double row_position = PE_TERRAIN_MAP_CENTRE_TILE - x / (double)PE_TERRAIN_TILE_SIZE;
  double column_position = PE_TERRAIN_MAP_CENTRE_TILE - y / (double)PE_TERRAIN_TILE_SIZE;

  int tile_y = (int)floor(row_position);
  int tile_x = (int)floor(column_position);

  const PTerrainWorldTile *tile = find_tile(world, tile_x, tile_y);
  if (tile == NULL)
    return false;

  return pe_terrain_heights_at(
      &tile->heights, (row_position - tile_y) * PE_TERRAIN_STEPS_PER_TILE,
      (column_position - tile_x) * PE_TERRAIN_STEPS_PER_TILE, height);
}

u32 pe_vk_terrain_world_draw(PTerrainWorld *world, const PTerrainFrame *frame,
                             VkCommandBuffer command, u32 image_index) {
  u32 drawn = 0;

  pe_vk_terrain_frame_update(&world->frames, image_index, frame);
  pe_vk_terrain_sky_draw(&world->pipeline, &world->frames, command,
                         image_index);

  for (u32 i = 0; i < world->tile_count; i++) {
    PTerrainDrawInfo draw = {.pipeline = &world->pipeline,
                             .frames = &world->frames,
                             .mesh = &world->tiles[i].mesh,
                             .materials = &world->tiles[i].materials,
                             .frame = frame,
                             .command_buffer = command,
                             .image_index = image_index};
    drawn += pe_vk_terrain_draw(&draw);
  }

  for (u32 i = 0; i < world->tile_count; i++)
    pe_vk_terrain_water_draw(&world->pipeline, &world->frames,
                             &world->tiles[i].water, command, image_index);

  return drawn;
}
