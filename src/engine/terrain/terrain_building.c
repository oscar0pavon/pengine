#include "terrain_building.h"

#include <engine/log.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WWB_MAGIC 0x32425757
#define GROUP_VERTICES_MAX 65536
#define BUILDING_INDICES_MAX (16 * 1024 * 1024)

static bool read_exact(FILE *file, void *out, size_t bytes) {
  return bytes == 0 || fread(out, 1, bytes, file) == bytes;
}

static bool read_u32(FILE *file, u32 *value) {
  return read_exact(file, value, sizeof(*value));
}

static bool read_textures(FILE *file, PBuilding *building) {
  if (read_u32(file, &building->texture_count) == false ||
      building->texture_count > PE_BUILDING_TEXTURES_MAX)
    return false;

  for (u32 i = 0; i < building->texture_count; i++) {
    u16 length;
    if (read_exact(file, &length, sizeof(length)) == false ||
        length >= PE_TERRAIN_BUILDING_PATH_MAX ||
        read_exact(file, building->textures[i], length) == false)
      return false;
    building->textures[i][length] = 0;
  }
  return true;
}

static bool read_materials(FILE *file, PBuilding *building) {
  if (read_u32(file, &building->material_count) == false ||
      building->material_count > PE_BUILDING_MATERIALS_MAX)
    return false;

  for (u32 i = 0; i < building->material_count; i++) {
    PBuildingMaterial *material = &building->materials[i];

    if (read_exact(file, material, sizeof(*material)) == false)
      return false;

    if (material->texture != PE_BUILDING_NO_TEXTURE &&
        material->texture >= building->texture_count)
      return false;
  }
  return true;
}

//a group is one room or wing. its indices count from its own first vertex,
//and once it is joined to the others they have to count from the first of all
//what a batch is in the file, before it is told which group it belongs to
typedef struct BatchRecord {
  u32 first_index;
  u32 index_count;
  u32 material;
} BatchRecord;

static bool read_group(FILE *file, PBuilding *building, u32 group_index) {
  PBuildingGroup *group = &building->groups[group_index];
  u32 counts[3];

  if (read_exact(file, &group->flags, sizeof(group->flags)) == false ||
      read_exact(file, group->bounds, sizeof(group->bounds)) == false ||
      read_exact(file, counts, sizeof(counts)) == false)
    return false;

  u32 vertex_count = counts[0];
  u32 index_count = counts[1];
  u32 batch_count = counts[2];

  if (vertex_count > GROUP_VERTICES_MAX ||
      (u64)building->index_count + index_count > BUILDING_INDICES_MAX)
    return false;

  u32 first_vertex = building->vertex_count;
  u32 first_index = building->index_count;
  u32 first_batch = building->batch_count;

  building->vertices = realloc(
      building->vertices, (first_vertex + vertex_count) * sizeof(PBuildingVertex));
  building->indices =
      realloc(building->indices, (first_index + index_count) * sizeof(u32));
  building->batches = realloc(
      building->batches, (first_batch + batch_count) * sizeof(PBuildingBatch));

  BatchRecord *records = malloc((batch_count ? batch_count : 1) * sizeof(*records));

  bool read = read_exact(file, &building->vertices[first_vertex],
                         vertex_count * sizeof(PBuildingVertex)) &&
              read_exact(file, &building->indices[first_index],
                         index_count * sizeof(u32)) &&
              read_exact(file, records, batch_count * sizeof(*records));
  if (read == false) {
    free(records);
    return false;
  }

  for (u32 i = 0; i < index_count; i++) {
    if (building->indices[first_index + i] >= vertex_count)
      return false;
    building->indices[first_index + i] += first_vertex;
  }

  for (u32 i = 0; i < batch_count; i++) {
    PBuildingBatch *batch = &building->batches[first_batch + i];

    if ((u64)records[i].first_index + records[i].index_count > index_count ||
        records[i].material >= building->material_count) {
      free(records);
      return false;
    }

    batch->first_index = records[i].first_index + first_index;
    batch->index_count = records[i].index_count;
    batch->material = records[i].material;
    batch->group = group_index;
  }
  free(records);

  building->vertex_count += vertex_count;
  building->index_count += index_count;
  building->batch_count += batch_count;
  return true;
}

static bool read_building(FILE *file, PBuilding *building) {
  u32 magic;
  u32 group_count;

  if (read_u32(file, &magic) == false || magic != WWB_MAGIC ||
      read_exact(file, building->bounds, sizeof(building->bounds)) == false ||
      read_textures(file, building) == false ||
      read_materials(file, building) == false ||
      read_u32(file, &group_count) == false ||
      group_count > PE_BUILDING_GROUPS_MAX)
    return false;

  building->group_count = group_count;
  for (u32 i = 0; i < group_count; i++)
    if (read_group(file, building, i) == false)
      return false;

  return building->vertex_count > 0 && building->batch_count > 0;
}

bool pe_building_load(const char *path, PBuilding *building) {
  memset(building, 0, sizeof(*building));

  FILE *file = fopen(path, "rb");
  if (file == NULL) {
    LOG("terrain: can't open %s\n", path);
    return false;
  }

  bool valid = read_building(file, building);
  fclose(file);

  if (valid == false) {
    LOG("terrain: %s is not a usable building file\n", path);
    pe_building_free(building);
  }
  return valid;
}

void pe_building_free(PBuilding *building) {
  free(building->vertices);
  free(building->indices);
  free(building->batches);
  memset(building, 0, sizeof(*building));
}

void pe_terrain_placement_matrix(const PTerrainPlacement *placement,
                                 mat4 matrix) {
  float about_x = glm_rad(placement->rotation[2]);
  float about_y = glm_rad(placement->rotation[0]);
  float about_z = glm_rad(placement->rotation[1] + 180.0f);

  glm_translate_make(matrix, (float *)placement->position);
  glm_scale(matrix, (vec3){1, -1, 1});
  glm_rotate_z(matrix, about_z, matrix);
  glm_rotate_y(matrix, about_y, matrix);
  glm_rotate_x(matrix, about_x, matrix);
}

bool pe_building_group_is_room(const PBuildingGroup *group) {
  return (group->flags & PE_BUILDING_GROUP_INTERIOR) &&
         (group->flags & PE_BUILDING_GROUP_EXTERIOR) == 0;
}

bool pe_building_camera_in_a_room(const PBuildingGroup *groups, u32 count,
                                  const vec3 camera) {
  const float margin = 0.5f;
  bool has_exterior = false;

  for (u32 i = 0; i < count; i++)
    has_exterior |= pe_building_group_is_room(&groups[i]) == false;
  if (has_exterior == false)
    return true;

  for (u32 i = 0; i < count; i++) {
    const float *box = groups[i].bounds;

    if (pe_building_group_is_room(&groups[i]) == false)
      continue;

    if (camera[0] >= box[0] - margin && camera[0] <= box[3] + margin &&
        camera[1] >= box[1] - margin && camera[1] <= box[4] + margin &&
        camera[2] >= box[2] - margin && camera[2] <= box[5] + margin)
      return true;
  }
  return false;
}
