#ifndef PE_TERRAIN_BUILDINGS_H
#define PE_TERRAIN_BUILDINGS_H

#include "terrain_building.h"
#include "terrain_pipeline.h"
#include "terrain_textures.h"

#define PE_TERRAIN_GPU_BUILDINGS_MAX 512
#define PE_TERRAIN_INSTANCES_MAX 16384

//how many descriptor sets, one for each material of each building, the pool
//can hand out
#define PE_TERRAIN_MATERIAL_SETS_MAX 4096

//one kind of building or prop on the gpu. it is read from disk the first time a
//tile places it and kept for every placement after, in this tile or the next.
//a prop is a building of one group
typedef struct PTerrainGpuBuilding {
  char name[PE_TERRAIN_BUILDING_PATH_MAX];

  //false for one that could not be loaded, kept so it is not tried again for
  //every placement of it
  bool usable;

  PBuffer vertex_buffer;
  PBuffer index_buffer;
  u32 batch_count;
  PBuildingBatch *batches;

  //a sphere round the whole of it, the centre in its own axes and then the radius
  vec4 sphere;

  //whether any group is a room, which is what says the camera is to be looked
  //for in it
  bool has_rooms;

  //which batches are rooms is worked out from these
  u32 group_count;
  PBuildingGroup groups[PE_BUILDING_GROUPS_MAX];

  u32 material_count;
  VkDescriptorSet material_sets[PE_BUILDING_MATERIALS_MAX];
  float alpha_cutoffs[PE_BUILDING_MATERIALS_MAX];
} PTerrainGpuBuilding;

//one building or prop standing somewhere
typedef struct PTerrainBuildingInstance {
  u32 building;
  u32 unique_id;
  mat4 model;

  //a sphere round the whole of it where it stands, for leaving it out when it
  //cannot be seen
  vec4 sphere;
} PTerrainBuildingInstance;

typedef struct PTerrainBuildings {
  VkDescriptorPool pool;
  u32 sets_used;

  u32 building_count;
  PTerrainGpuBuilding buildings[PE_TERRAIN_GPU_BUILDINGS_MAX];

  //kept in the order of the buildings, so that one is bound once for all the
  //places it stands
  u32 instance_count;
  PTerrainBuildingInstance instances[PE_TERRAIN_INSTANCES_MAX];
} PTerrainBuildings;

//needs the renderer up, so from the game's init or later
void pe_vk_terrain_buildings_create(PTerrainBuildings *buildings);

//puts the buildings and props a tile places in the world. they themselves and
//their textures are read from directory, "world/wmo/.../x.wwb" and the like,
//and one that is not there is logged once and left out. a placement with the
//unique id of one already in the world is left out too, because a building
//that stands across two tiles is listed by each, and so is a prop
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
