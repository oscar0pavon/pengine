#ifndef PE_TERRAIN_BUILDINGS_H
#define PE_TERRAIN_BUILDINGS_H

#include "terrain_building.h"
#include "terrain_pipeline.h"
#include "terrain_textures.h"

#define PE_TERRAIN_GPU_BUILDINGS_MAX 64
#define PE_TERRAIN_INSTANCES_MAX 512

//how many descriptor sets, one for each material of each building, the pool
//can hand out
#define PE_TERRAIN_MATERIAL_SETS_MAX 4096

//one kind of building on the gpu. it is read from disk the first time a tile
//places it and kept for every placement after, in this tile or the next
typedef struct PTerrainGpuBuilding {
  char name[PE_TERRAIN_BUILDING_PATH_MAX];

  //false for one that could not be loaded, kept so it is not tried again for
  //every placement of it
  bool usable;

  PBuffer vertex_buffer;
  PBuffer index_buffer;
  u32 batch_count;
  PBuildingBatch *batches;

  //which batches are rooms is worked out from these
  u32 group_count;
  PBuildingGroup groups[PE_BUILDING_GROUPS_MAX];

  u32 material_count;
  VkDescriptorSet material_sets[PE_BUILDING_MATERIALS_MAX];
  float alpha_cutoffs[PE_BUILDING_MATERIALS_MAX];
} PTerrainGpuBuilding;

//one building standing somewhere
typedef struct PTerrainBuildingInstance {
  u32 building;
  u32 unique_id;
  mat4 model;

  //a sphere round the whole of it, for leaving it out when it cannot be seen
  vec4 sphere;
} PTerrainBuildingInstance;

typedef struct PTerrainBuildings {
  VkDescriptorPool pool;
  u32 sets_used;

  u32 building_count;
  PTerrainGpuBuilding buildings[PE_TERRAIN_GPU_BUILDINGS_MAX];

  u32 instance_count;
  PTerrainBuildingInstance instances[PE_TERRAIN_INSTANCES_MAX];
} PTerrainBuildings;

//needs the renderer up, so from the game's init or later
void pe_vk_terrain_buildings_create(PTerrainBuildings *buildings);

//puts the buildings a tile places in the world. the buildings themselves and
//their textures are read from directory, "world/wmo/.../x.wwb" and the like,
//and a building that is not there is logged once and left out. a placement
//with the unique id of one already in the world is left out too, because a
//building that stands across two tiles is listed by each
void pe_vk_terrain_buildings_add_tile(const PTerrainPipeline *pipeline,
                                      PTerrainTextures *textures,
                                      PTerrainBuildings *buildings,
                                      const PTerrainTile *tile,
                                      const char *directory);

//records every building the camera can see, and returns how many. a building
//past the distance where the fog hides everything is not drawn
u32 pe_vk_terrain_buildings_draw(const PTerrainPipeline *pipeline,
                                 const PTerrainFrames *frames,
                                 const PTerrainBuildings *buildings,
                                 const PTerrainFrame *frame,
                                 VkCommandBuffer command, u32 image_index);

#endif // !PE_TERRAIN_BUILDINGS_H
