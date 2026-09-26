#include "terrain.h"

#include <ThirdParty/parson.h>
#include <engine/log.h>
#include <engine/macros.h>

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define WHM_MAGIC 0x314D4857
#define WWT_MAGIC 0x31545757
#define WHM_ALPHA_BLOB_MAX 65536
#define ALPHA_PACKED_SIZE (PE_TERRAIN_ALPHA_SIZE / 2)

static bool sibling_path(char *path, const char *base_path,
                         const char *extension) {
  int length = snprintf(path, PATH_MAX, "%s%s", base_path, extension);
  return length > 0 && length < PATH_MAX;
}

static bool tile_in_grid(int tile) {
  return tile >= 0 && tile < PE_TERRAIN_TILES_PER_SIDE;
}

static u16 holes_from_json(double value) {
  return value >= 0 && value <= UINT16_MAX ? (u16)value : 0;
}

static bool read_textures(const JSON_Object *root, PTerrainTile *tile) {
  const JSON_Array *names = json_object_get_array(root, "textures");
  size_t count = json_array_get_count(names);

  if (count > PE_TERRAIN_TEXTURES_MAX) {
    LOG("terrain: %zu textures, the most a tile takes is %d\n", count,
        PE_TERRAIN_TEXTURES_MAX);
    return false;
  }

  for (size_t i = 0; i < count; i++) {
    const char *name = json_array_get_string(names, i);
    if (name == NULL)
      continue;

    if (strlen(name) >= PE_TERRAIN_TEXTURE_PATH_MAX) {
      LOG("terrain: texture path too long: %s\n", name);
      return false;
    }
    strcpy(tile->textures[i], name);
  }

  tile->texture_count = count;
  return true;
}

static bool read_chunk_layers(const JSON_Object *root, PTerrainTile *tile) {
  const JSON_Array *entries = json_object_get_array(root, "chunkLayers");
  size_t count = json_array_get_count(entries);
  if (count > PE_TERRAIN_CHUNKS)
    count = PE_TERRAIN_CHUNKS;

  for (size_t i = 0; i < count; i++) {
    const JSON_Object *entry = json_array_get_object(entries, i);
    const JSON_Array *layers = json_object_get_array(entry, "layers");
    PTerrainChunk *chunk = &tile->chunks[i];

    size_t layer_count = json_array_get_count(layers);
    if (layer_count > PE_TERRAIN_LAYERS_MAX)
      layer_count = PE_TERRAIN_LAYERS_MAX;

    for (size_t j = 0; j < layer_count; j++) {
      double texture = json_array_get_number(layers, j);
      if (texture < 0 || texture >= tile->texture_count) {
        LOG("terrain: chunk %zu layer %zu names texture %.0f, tile has %u\n",
            i, j, texture, tile->texture_count);
        return false;
      }
      chunk->layer_textures[j] = (u32)texture;
    }

    chunk->layer_count = layer_count;
    chunk->holes = holes_from_json(json_object_get_number(entry, "holes"));
  }
  return true;
}

static bool read_metadata(const char *path, PTerrainTile *tile) {
  JSON_Value *document = json_parse_file(path);
  const JSON_Object *root = json_value_get_object(document);
  if (root == NULL) {
    LOG("terrain: can't read %s\n", path);
    json_value_free(document);
    return false;
  }

  tile->tile_x = (int)json_object_get_number(root, "tileX");
  tile->tile_y = (int)json_object_get_number(root, "tileY");

  bool valid = tile_in_grid(tile->tile_x) && tile_in_grid(tile->tile_y);
  if (valid == false)
    LOG("terrain: tile %d,%d is outside the %dx%d grid\n", tile->tile_x,
        tile->tile_y, PE_TERRAIN_TILES_PER_SIDE, PE_TERRAIN_TILES_PER_SIDE);

  valid = valid && read_textures(root, tile) && read_chunk_layers(root, tile);

  json_value_free(document);
  return valid;
}

//a four bit map has 63 painted columns and rows, the last of each repeats the
//one before it
static void repeat_last_row_and_column(u8 *alpha) {
  const int last = PE_TERRAIN_ALPHA_DIM - 1;

  for (int i = 0; i < PE_TERRAIN_ALPHA_DIM; i++)
    alpha[last * PE_TERRAIN_ALPHA_DIM + i] =
        alpha[(last - 1) * PE_TERRAIN_ALPHA_DIM + i];

  for (int i = 0; i < PE_TERRAIN_ALPHA_DIM; i++)
    alpha[i * PE_TERRAIN_ALPHA_DIM + last] =
        alpha[i * PE_TERRAIN_ALPHA_DIM + last - 1];
}

static void unpack_four_bit_alpha(const u8 *packed, u8 *alpha) {
  for (int i = 0; i < ALPHA_PACKED_SIZE; i++) {
    alpha[i * 2] = (packed[i] & 0x0F) * 17;
    alpha[i * 2 + 1] = (packed[i] >> 4) * 17;
  }
  repeat_last_row_and_column(alpha);
}

//INFO the .wot lists a chunk's layers but not where their maps start, so the
//blob is read as one map per layer above the base, back to back, all the same
//size. that size says whether they are eight bit or four bit. the compressed
//form of the original format cannot be told apart this way and is not read
static void decode_alpha(PTerrainChunk *chunk, const u8 *blob, u32 size) {
  if (chunk->layer_count < 2)
    return;

  u32 maps = chunk->layer_count - 1;
  bool eight_bit = size >= maps * PE_TERRAIN_ALPHA_SIZE;
  bool four_bit = size >= maps * ALPHA_PACKED_SIZE;

  if (eight_bit == false && four_bit == false) {
    if (size > 0)
      LOG("terrain: %u alpha bytes is too few for %u layers\n", size,
          chunk->layer_count);
    return;
  }

  for (u32 i = 0; i < maps; i++) {
    if (eight_bit)
      memcpy(chunk->layer_alpha[i], blob + i * PE_TERRAIN_ALPHA_SIZE,
             PE_TERRAIN_ALPHA_SIZE);
    else
      unpack_four_bit_alpha(blob + i * ALPHA_PACKED_SIZE,
                            chunk->layer_alpha[i]);
  }
}

static bool read_chunk(FILE *file, PTerrainChunk *chunk) {
  u8 alpha_blob[WHM_ALPHA_BLOB_MAX];
  u32 alpha_size = 0;

  if (fread(&chunk->base_height, sizeof(float), 1, file) != 1)
    return false;
  if (fread(chunk->heights, sizeof(float), PE_TERRAIN_CHUNK_VERTICES, file) !=
      PE_TERRAIN_CHUNK_VERTICES)
    return false;

  if (isfinite(chunk->base_height) == false)
    chunk->base_height = 0;
  for (int i = 0; i < PE_TERRAIN_CHUNK_VERTICES; i++)
    if (isfinite(chunk->heights[i]) == false)
      chunk->heights[i] = 0;

  //files from before the alpha maps existed end here
  if (fread(&alpha_size, sizeof(u32), 1, file) != 1)
    return true;

  if (alpha_size > WHM_ALPHA_BLOB_MAX) {
    LOG("terrain: alpha size %u is over the %d byte limit\n", alpha_size,
        WHM_ALPHA_BLOB_MAX);
    return false;
  }
  if (fread(alpha_blob, 1, alpha_size, file) != alpha_size)
    return false;

  decode_alpha(chunk, alpha_blob, alpha_size);
  return true;
}

static bool read_heightmap(const char *path, PTerrainTile *tile) {
  FILE *file = fopen(path, "rb");
  if (file == NULL) {
    LOG("terrain: can't open %s\n", path);
    return false;
  }

  u32 header[3];
  bool valid = fread(header, sizeof(u32), 3, file) == 3 &&
               header[0] == WHM_MAGIC && header[1] == PE_TERRAIN_CHUNKS &&
               header[2] == PE_TERRAIN_CHUNK_VERTICES;
  if (valid == false)
    LOG("terrain: %s is not a %d chunk, %d vertex WHM file\n", path,
        PE_TERRAIN_CHUNKS, PE_TERRAIN_CHUNK_VERTICES);

  for (int i = 0; valid && i < PE_TERRAIN_CHUNKS; i++) {
    valid = read_chunk(file, &tile->chunks[i]);
    if (valid == false)
      LOG("terrain: %s ends inside chunk %d\n", path, i);
  }

  fclose(file);
  return valid;
}

static bool read_water_chunk(FILE *file, PTerrainTile *tile) {
  u32 index;
  u32 type;
  if (fread(&index, sizeof(u32), 1, file) != 1 ||
      fread(&type, sizeof(u32), 1, file) != 1)
    return false;

  if (index >= PE_TERRAIN_CHUNKS || type > PE_TERRAIN_LIQUID_SLIME) {
    LOG("terrain: water names chunk %u of type %u\n", index, type);
    return false;
  }

  PTerrainChunkWater *water = &tile->water[index];
  water->present = true;
  water->type = type;

  if (fread(water->heights, sizeof(float), PE_TERRAIN_WATER_VERTICES, file) !=
          PE_TERRAIN_WATER_VERTICES ||
      fread(water->depths, 1, PE_TERRAIN_WATER_VERTICES, file) !=
          PE_TERRAIN_WATER_VERTICES ||
      fread(water->visible, 1, PE_TERRAIN_WATER_QUADS, file) !=
          PE_TERRAIN_WATER_QUADS)
    return false;

  for (int i = 0; i < PE_TERRAIN_WATER_VERTICES; i++)
    if (isfinite(water->heights[i]) == false)
      water->heights[i] = 0;
  return true;
}

//a tile with no water has no file, which is not an error
static bool read_water(const char *path, PTerrainTile *tile) {
  FILE *file = fopen(path, "rb");
  if (file == NULL)
    return true;

  u32 header[2];
  bool valid = fread(header, sizeof(u32), 2, file) == 2 &&
               header[0] == WWT_MAGIC && header[1] <= PE_TERRAIN_CHUNKS;
  if (valid == false)
    LOG("terrain: %s is not a WWT file\n", path);

  for (u32 i = 0; valid && i < header[1]; i++) {
    valid = read_water_chunk(file, tile);
    if (valid == false)
      LOG("terrain: %s ends inside water chunk %u\n", path, i);
  }

  fclose(file);
  return valid;
}

//INFO the metadata goes first because the layers it lists are what tell
//read_chunk how to split each chunk's alpha blob
bool pe_terrain_load(const char *base_path, PTerrainTile *tile) {
  char metadata_path[PATH_MAX];
  char heightmap_path[PATH_MAX];
  char water_path[PATH_MAX];

  if (sibling_path(metadata_path, base_path, ".wot") == false ||
      sibling_path(heightmap_path, base_path, ".whm") == false ||
      sibling_path(water_path, base_path, ".wwt") == false) {
    LOG("terrain: path too long: %s\n", base_path);
    return false;
  }

  ZERO(*tile);
  return read_metadata(metadata_path, tile) &&
         read_heightmap(heightmap_path, tile) && read_water(water_path, tile);
}
