#ifndef PE_TERRAIN_BUILDING_H
#define PE_TERRAIN_BUILDING_H

#include "terrain.h"

#include <cglm/cglm.h>

#define PE_BUILDING_TEXTURES_MAX 256
#define PE_BUILDING_MATERIALS_MAX 256

//a material that names no texture
#define PE_BUILDING_NO_TEXTURE 0xFFFFFFFFu

//how a material's alpha is used, the game's own numbers for it: 0 draws it as
//it is, 1 throws away what is mostly transparent, which is how a window or a
//leaf is cut out of a quad, 2 blends it over what is behind, as glass or a
//petal, and 3 adds it to what is behind, as a candle's glow or a shaft of
//light. anything above that is drawn as 0
#define PE_BUILDING_BLEND_OPAQUE 0
#define PE_BUILDING_BLEND_ALPHA_TEST 1
#define PE_BUILDING_BLEND_ALPHA 2
#define PE_BUILDING_BLEND_ADD 3

typedef struct PBuildingVertex {
  float position[3];
  float normal[3];
  float uv[2];

  //red, green, blue and alpha. the light the building's makers painted into
  //its rooms, white where they painted none
  u8 color[4];
} PBuildingVertex;

_Static_assert(sizeof(PBuildingVertex) == 36,
               "the pipeline reads this as nine words with no padding");

//one run of indices drawn with one material
typedef struct PBuildingBatch {
  u32 first_index;
  u32 index_count;
  u32 material;
} PBuildingBatch;

#define PE_BUILDING_GROUPS_MAX 64

//the file keeps each group's flags and box. they are read and not used to
//draw: every room is drawn, since a triangle seen from behind is culled, and
//the inside of a wall is behind
typedef struct PBuildingGroup {
  u32 flags;

  //the box round the group, low corner then high, in the building's own axes
  float bounds[6];
} PBuildingGroup;

//a material flag, the same bit in a building's materials and a model's: the
//back of its triangles is drawn too
#define PE_BUILDING_MATERIAL_TWO_SIDED 0x4

typedef struct PBuildingMaterial {
  u32 texture;
  u32 blend;
  u32 flags;
} PBuildingMaterial;

//a building with its groups, the rooms and wings the game splits it into,
//joined into one set of vertices and indices
typedef struct PBuilding {
  //the box around all the groups, in the building's own axes
  float bounds[6];

  u32 texture_count;
  char textures[PE_BUILDING_TEXTURES_MAX][PE_TERRAIN_BUILDING_PATH_MAX];

  u32 material_count;
  PBuildingMaterial materials[PE_BUILDING_MATERIALS_MAX];

  u32 group_count;
  PBuildingGroup groups[PE_BUILDING_GROUPS_MAX];

  u32 vertex_count;
  PBuildingVertex *vertices;
  u32 index_count;
  u32 *indices;
  u32 batch_count;
  PBuildingBatch *batches;
} PBuilding;

//the props inside a building, its tables and lamps and barrels, read from the
//.wwd beside its .wwb. each is a model, a .wwb of its own, put at a place in the
//building's own axes. a placement of the building chooses one set of them, and
//set 0 goes with every placement
typedef struct PBuildingDoodad {
  u32 model;
  float position[3];

  //x, y, z, w
  float rotation[4];
  float scale;
} PBuildingDoodad;

_Static_assert(sizeof(PBuildingDoodad) == 36,
               "the file holds these as nine words with no padding");

typedef struct PBuildingDoodadSet {
  u32 first;
  u32 count;
} PBuildingDoodadSet;

typedef struct PBuildingDoodads {
  u32 model_count;
  char (*models)[PE_TERRAIN_BUILDING_PATH_MAX];

  u32 set_count;
  PBuildingDoodadSet *sets;

  u32 count;
  PBuildingDoodad *items;
} PBuildingDoodads;

//reads a .wwd. a building without one, and a prop never has one, is one with
//no props inside, and that is true with nothing left allocated. false for a
//file that is not a .wwd or that points outside itself
bool pe_building_doodads_load(const char *path, PBuildingDoodads *doodads);

void pe_building_doodads_free(PBuildingDoodads *doodads);

//where a prop inside a building is in the world: the building's matrix, and in
//front of it the prop's own place in the building
void pe_building_doodad_matrix(const mat4 building, const PBuildingDoodad *doodad,
                               mat4 matrix);

//reads a .wwb, the building format the converter writes. false, with nothing
//left allocated, for a file that is not one or that points outside itself
bool pe_building_load(const char *path, PBuilding *building);

void pe_building_free(PBuilding *building);

//INFO where the game puts a building and which way it faces. the rule is not
//obvious and was not guessed: it is the one that reproduces, for every
//building in the Goldshire tiles, the box the game's own tools stored for it.
//translate, then turn about Z, Y and X, by the three degrees taken in the order
//(rotation[2], rotation[0], rotation[1] + 180). the +180 is not optional: left
//out, the boxes are ten yards out. in front of all that sits the reflection of
//Y that makes the world east and not west, so the building's own vertices stay
//as the game wrote them. the scale of a prop comes last, so it grows about its
//own origin
void pe_terrain_placement_matrix(const PTerrainPlacement *placement,
                                 mat4 matrix);

#endif // !PE_TERRAIN_BUILDING_H
