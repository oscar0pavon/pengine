#include "terrain_world.h"

#include <engine/log.h>
#include <engine/renderer/vulkan.h>

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

void pe_vk_terrain_world_create(PTerrainWorld *world) {
  pe_vk_terrain_pipeline_create(&world->pipeline);
  pe_vk_terrain_frames_create(&world->pipeline, &world->frames);
  pe_vk_terrain_buildings_create(&world->buildings);
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

static const PTerrainWorldTile *find_tile(const PTerrainWorld *world,
                                          int tile_x, int tile_y) {
  for (u32 i = 0; i < world->tile_count; i++)
    if (world->tiles[i].tile_x == tile_x && world->tiles[i].tile_y == tile_y)
      return &world->tiles[i];
  return NULL;
}

static bool is_loaded(const PTerrainWorld *world, int tile_x, int tile_y) {
  return find_tile(world, tile_x, tile_y) != NULL;
}

//the tile and the eight around it, read from disk to build the tile's mesh: its
//border is lit with the heights across it, and a neighbour that is loaded, or
//that is not loaded yet, has to give the same ones
typedef struct Surround {
  PTerrainTile *tiles[3][3];
} Surround;

static void read_surround(const char *directory, const char *map, int tile_x,
                          int tile_y, Surround *surround) {
  for (int row = 0; row < 3; row++)
    for (int column = 0; column < 3; column++)
      surround->tiles[row][column] =
          read_tile(directory, map, tile_x + column - 1, tile_y + row - 1);
}

static void free_surround(Surround *surround) {
  for (int row = 0; row < 3; row++)
    for (int column = 0; column < 3; column++)
      free(surround->tiles[row][column]);
}

static void upload_tile(PTerrainWorld *world, const Surround *surround,
                        const char *directory) {
  const PTerrainTile *tile = surround->tiles[1][1];

  PTerrainNeighbours neighbours;
  for (int row = 0; row < 3; row++)
    for (int column = 0; column < 3; column++)
      neighbours.tiles[row][column] = surround->tiles[row][column];

  PTerrainMesh *mesh = malloc(sizeof(PTerrainMesh));
  PTerrainWorldTile *out = &world->tiles[world->tile_count++];
  out->tile_x = tile->tile_x;
  out->tile_y = tile->tile_y;

  pe_terrain_mesh_build(tile, &neighbours, mesh);
  pe_vk_terrain_mesh_upload(mesh, &out->mesh);
  free(mesh);

  pe_vk_terrain_materials_create(&world->pipeline, &world->textures, tile,
                                 directory, &out->materials);
  pe_vk_terrain_water_upload(tile, &out->water);
  pe_terrain_heights_from_tile(tile, &out->heights);
  pe_vk_terrain_buildings_add_tile(&world->pipeline, &world->textures,
                                   &world->buildings, tile, directory);
}

static double seconds_now(void) {
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  return now.tv_sec + now.tv_nsec / 1e9;
}

static bool load_tile(PTerrainWorld *world, const char *directory,
                      const char *map, int tile_x, int tile_y) {
  double started = seconds_now();

  Surround surround;
  read_surround(directory, map, tile_x, tile_y, &surround);

  bool loaded = surround.tiles[1][1] != NULL;
  if (loaded)
    upload_tile(world, &surround, directory);
  else
    world->unavailable[tile_y][tile_x] = true;

  free_surround(&surround);
  if (loaded)
    LOG("terrain: tile %d,%d loaded in %.0f ms\n", tile_x, tile_y,
        (seconds_now() - started) * 1000);
  return loaded;
}

static void unload_tile(PTerrainWorld *world, u32 index) {
  PTerrainWorldTile *tile = &world->tiles[index];

  pe_vk_terrain_mesh_free(&tile->mesh);
  pe_vk_terrain_materials_free(&world->textures, &tile->materials);
  pe_vk_terrain_water_free(&tile->water);
  pe_vk_terrain_buildings_remove_tile(&world->buildings, &world->textures,
                                      tile->tile_x, tile->tile_y);

  LOG("terrain: tile %d,%d unloaded\n", tile->tile_x, tile->tile_y);
  *tile = world->tiles[--world->tile_count];
}

//how far x is from the range low to high, 0 inside it
static double gap_to(double x, double low, double high) {
  if (x < low)
    return low - x;
  return x > high ? x - high : 0;
}

//how far a world position is from the ground of a tile, the rectangle it covers.
//rows run toward -X and columns toward +Y, from the tile whose near corner is
//the middle of the map
static double distance_to_tile(double x, double y, int tile_x, int tile_y) {
  double size = PE_TERRAIN_TILE_SIZE;
  double south = (PE_TERRAIN_MAP_CENTRE_TILE - tile_y - 1) * size;
  double west = (tile_x - PE_TERRAIN_MAP_CENTRE_TILE) * size;

  return hypot(gap_to(x, south, south + size), gap_to(y, west, west + size));
}

static bool unload_far_tiles(PTerrainWorld *world, float x, float y,
                             float distance) {
  bool unloaded = false;

  for (u32 i = 0; i < world->tile_count;) {
    const PTerrainWorldTile *tile = &world->tiles[i];

    if (distance_to_tile(x, y, tile->tile_x, tile->tile_y) <= distance) {
      i++;
      continue;
    }

    //what was recorded for the frame in flight may still use the tile
    if (unloaded == false)
      vkDeviceWaitIdle(vk_device);
    unload_tile(world, i);
    unloaded = true;
  }
  return unloaded;
}

//the nearest tile within distance that is not loaded and the game has
static bool find_tile_to_load(PTerrainWorld *world, const char *directory,
                              const char *map, float x, float y,
                              float distance, int *tile_x, int *tile_y) {
  int reach = (int)ceil(distance / PE_TERRAIN_TILE_SIZE);
  int centre_y = (int)floor(PE_TERRAIN_MAP_CENTRE_TILE - x / (double)PE_TERRAIN_TILE_SIZE);
  int centre_x = (int)floor(PE_TERRAIN_MAP_CENTRE_TILE + y / (double)PE_TERRAIN_TILE_SIZE);

  for (;;) {
    double nearest = distance;
    bool found = false;

    for (int row = centre_y - reach; row <= centre_y + reach; row++) {
      for (int column = centre_x - reach; column <= centre_x + reach; column++) {
        if (row < 0 || column < 0 || row >= PE_TERRAIN_TILES_PER_SIDE ||
            column >= PE_TERRAIN_TILES_PER_SIDE ||
            world->unavailable[row][column] || is_loaded(world, column, row))
          continue;

        double away = distance_to_tile(x, y, column, row);
        if (away > nearest)
          continue;

        nearest = away;
        *tile_x = column;
        *tile_y = row;
        found = true;
      }
    }

    if (found == false)
      return false;
    if (tile_files_exist(directory, map, *tile_x, *tile_y))
      return true;
    world->unavailable[*tile_y][*tile_x] = true;
  }
}

bool pe_vk_terrain_world_stream(PTerrainWorld *world, const char *directory,
                                const char *map, float x, float y,
                                float distance) {
  bool changed =
      unload_far_tiles(world, x, y, distance + PE_TERRAIN_STREAM_MARGIN);

  int tile_x, tile_y;
  if (world->tile_count < PE_TERRAIN_WORLD_TILES_MAX &&
      find_tile_to_load(world, directory, map, x, y, distance, &tile_x,
                        &tile_y))
    changed |= load_tile(world, directory, map, tile_x, tile_y);

  return changed;
}

//INFO x and y are worked back to tiles in double. the tile size times a few
//dozen tiles is a number in the thousands, and a float has about a thousandth
//of a yard left at that size, which is a tenth of a step of the ground
bool pe_terrain_world_height_at(const PTerrainWorld *world, float x, float y,
                                float *height) {
  double row_position = PE_TERRAIN_MAP_CENTRE_TILE - x / (double)PE_TERRAIN_TILE_SIZE;
  double column_position = PE_TERRAIN_MAP_CENTRE_TILE + y / (double)PE_TERRAIN_TILE_SIZE;

  int tile_y = (int)floor(row_position);
  int tile_x = (int)floor(column_position);

  const PTerrainWorldTile *tile = find_tile(world, tile_x, tile_y);
  if (tile == NULL)
    return false;

  return pe_terrain_heights_at(
      &tile->heights, (row_position - tile_y) * PE_TERRAIN_STEPS_PER_TILE,
      (column_position - tile_x) * PE_TERRAIN_STEPS_PER_TILE, height);
}

bool pe_terrain_world_floor_at(const PTerrainWorld *world, float x, float y,
                               float z, float *height) {
  float ground, floor;
  bool on_ground = pe_terrain_world_height_at(world, x, y, &ground);
  bool on_floor =
      pe_terrain_buildings_floor_at(&world->buildings, (vec3){x, y, z}, &floor);

  if (on_ground == false && on_floor == false)
    return false;

  *height = on_ground && (on_floor == false || ground > floor) ? ground : floor;
  return true;
}

bool pe_terrain_world_push_out(const PTerrainWorld *world, vec3 centre,
                               float radius) {
  return pe_terrain_buildings_push_out(&world->buildings, centre, radius);
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

  world->buildings_drawn = pe_vk_terrain_buildings_draw(
      &world->pipeline, &world->frames, &world->buildings, frame, command,
      image_index);

  for (u32 i = 0; i < world->tile_count; i++)
    pe_vk_terrain_water_draw(&world->pipeline, &world->frames,
                             &world->tiles[i].water, command, image_index);

  return drawn;
}
