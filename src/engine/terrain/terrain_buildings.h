#ifndef PE_TERRAIN_BUILDINGS_H
#define PE_TERRAIN_BUILDINGS_H

#include "terrain_building.h"
#include "terrain_collision.h"
#include "terrain_pipeline.h"
#include "terrain_textures.h"

#define PE_TERRAIN_GPU_BUILDINGS_MAX 1024
#define PE_TERRAIN_INSTANCES_MAX 65536

//how many descriptor sets, one for each material of each building, the pool
//can hand out
#define PE_TERRAIN_MATERIAL_SETS_MAX 8192

//how many tiles can list the same building at once. one that stands on the
//corner of four is listed by all four
#define PE_TERRAIN_INSTANCE_OWNERS_MAX 4

//one kind of building or prop on the gpu. it is read from disk the first time a
//tile places it and kept for as long as some placement of it stands, in this
//tile or the next, and given back when the last one goes.
//a prop is a building of one group
typedef struct PTerrainGpuBuilding {
  //false for a slot nothing is loaded in
  bool in_use;

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

  //the props inside it, none for most
  PBuildingDoodads doodads;

  //what can be walked on and into. a building has all it is drawn with, a prop
  //the few triangles of its own, and one that is walked through has none
  PCollisionMesh collision;

  u32 material_count;
  VkDescriptorSet material_sets[PE_BUILDING_MATERIALS_MAX];
  const PTexture *material_textures[PE_BUILDING_MATERIALS_MAX];
  float alpha_cutoffs[PE_BUILDING_MATERIALS_MAX];

  //how each material is drawn, and whether any is blended
  PBuildingShader shaders[PE_BUILDING_MATERIALS_MAX];
  bool has_blended;
} PTerrainGpuBuilding;

//one building or prop standing somewhere
typedef struct PTerrainBuildingInstance {
  u32 building;

  //the game's id of the placement it is from, and for a prop inside a building
  //that of the building. 0 if it has none
  u32 unique_id;

  //the tiles that list it, as their pe_terrain_tile_id(). it stands for as long
  //as one of them is loaded
  u16 owners[PE_TERRAIN_INSTANCE_OWNERS_MAX];
  u8 owner_count;

  mat4 model;

  //a sphere round the whole of it where it stands, for leaving it out when it
  //cannot be seen
  vec4 sphere;
} PTerrainBuildingInstance;

//a building with something blended in it that can be seen, and how far off, to
//draw those after everything solid, the far ones first
typedef struct PTerrainBlendedInstance {
  u32 instance;
  float distance;
} PTerrainBlendedInstance;

typedef struct PTerrainBuildings {
  VkDescriptorPool pool;
  u32 sets_used;

  u32 building_count;
  PTerrainGpuBuilding buildings[PE_TERRAIN_GPU_BUILDINGS_MAX];

  //kept in the order of the buildings, so that one is bound once for all the
  //places it stands
  u32 instance_count;
  PTerrainBuildingInstance instances[PE_TERRAIN_INSTANCES_MAX];

  u32 blended_count;
  PTerrainBlendedInstance blended[PE_TERRAIN_INSTANCES_MAX];
} PTerrainBuildings;

//needs the renderer up, so from the game's init or later
void pe_vk_terrain_buildings_create(PTerrainBuildings *buildings);

//puts the buildings and props a tile places in the world, and the props inside
//each building. they themselves and
//their textures are read from directory, "world/wmo/.../x.wwb" and the like,
//and one that is not there is logged once and left out. a placement with the
//unique id of one already in the world is left out too, because a building
//that stands across two tiles is listed by each, and so is a prop
void pe_vk_terrain_buildings_add_tile(const PTerrainPipeline *pipeline,
                                      PTerrainTextures *textures,
                                      PTerrainBuildings *buildings,
                                      const PTerrainTile *tile,
                                      const char *directory);

//takes away what a tile put in the world, which is every building and prop of
//it that no other loaded tile lists, and gives back the kinds of building that
//are left with no placement. the gpu must not be drawing any of it, so wait for
//it to be idle first
void pe_vk_terrain_buildings_remove_tile(PTerrainBuildings *buildings,
                                         PTerrainTextures *textures,
                                         int tile_x, int tile_y);

//how far below a point a floor is looked for
#define PE_TERRAIN_FLOOR_REACH 60.0f

//the height of the highest floor of a building or a prop that is at or below
//from, straight under it, and so the one a walker at from stands on. false if
//there is none within PE_TERRAIN_FLOOR_REACH
bool pe_terrain_buildings_floor_at(const PTerrainBuildings *buildings,
                                   const vec3 from, float *height);

//moves the centre of a sphere out of the walls of the buildings and props, and
//returns whether it moved. a floor or a ceiling is not a wall
bool pe_terrain_buildings_push_out(const PTerrainBuildings *buildings,
                                   vec3 centre, float radius);

//records every building the camera can see, and returns how many. a building
//past the distance where the fog hides everything is not drawn. what is solid
//in them is drawn first and what is blended over it after, the far ones first
u32 pe_vk_terrain_buildings_draw(const PTerrainPipeline *pipeline,
                                 const PTerrainFrames *frames,
                                 PTerrainBuildings *buildings,
                                 const PTerrainFrame *frame,
                                 VkCommandBuffer command, u32 image_index);

#endif // !PE_TERRAIN_BUILDINGS_H
