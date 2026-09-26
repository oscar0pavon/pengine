#ifndef PE_TERRAIN_BUILDING_H
#define PE_TERRAIN_BUILDING_H

#include "terrain.h"

#include <cglm/cglm.h>

#define PE_BUILDING_TEXTURES_MAX 256
#define PE_BUILDING_MATERIALS_MAX 256

//a material that names no texture
#define PE_BUILDING_NO_TEXTURE 0xFFFFFFFFu

//how a material's alpha is used: 0 draws it as it is and 1 throws away what is
//mostly transparent, which is how a window or a leaf is cut out of a quad. the
//buildings in the Goldshire tiles use nothing else
#define PE_BUILDING_BLEND_OPAQUE 0
#define PE_BUILDING_BLEND_ALPHA_TEST 1

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

  u32 vertex_count;
  PBuildingVertex *vertices;
  u32 index_count;
  u32 *indices;
  u32 batch_count;
  PBuildingBatch *batches;
} PBuilding;

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
//as the game wrote them
void pe_terrain_placement_matrix(const PTerrainPlacement *placement,
                                 mat4 matrix);

#endif // !PE_TERRAIN_BUILDING_H
