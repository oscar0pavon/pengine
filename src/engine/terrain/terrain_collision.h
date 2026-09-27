#ifndef PE_TERRAIN_COLLISION_H
#define PE_TERRAIN_COLLISION_H

#include "terrain_building.h"

#include <cglm/cglm.h>

//a triangle whose normal is within this of straight up or down, as the cosine
//of the angle, is a floor or a ceiling to walk on or under, and any steeper is a
//wall. it is about 53 degrees
#define PE_COLLISION_WALKABLE 0.6f

//the triangles of a building, in its own axes, with a grid over them so a
//question about one place looks at a few of them and not at all. a building of
//the game has a hundred thousand
typedef struct PCollisionMesh {
  u32 vertex_count;
  float (*positions)[3];

  u32 triangle_count;
  u32 (*triangles)[3];

  //the grid lies over the plane of X and Y, and each cell lists the triangles
  //that reach into it, which is how a triangle is in more than one cell
  float origin[2];
  float cell_size;
  u32 columns;
  u32 rows;
  u32 *cell_start;
  u32 *cell_triangles;
} PCollisionMesh;

//every triangle of the building, the ones it is drawn with and those it only
//collides with. false for a building with none
bool pe_collision_mesh_build(const PBuilding *source, PCollisionMesh *mesh);

void pe_collision_mesh_free(PCollisionMesh *mesh);

//the nearest floor that a ray meets, going from origin along down, a unit
//vector, for at most max_distance. only a triangle that can be walked on
//counts, judged against up, the way opposite to down. false if it meets none
bool pe_collision_mesh_floor(const PCollisionMesh *mesh, const vec3 origin,
                             const vec3 down, float max_distance,
                             float *distance);

//moves the centre of a sphere out of every wall it is inside, along the plane
//at right angles to up, so that a sphere held up by a floor is not lifted off
//it. floors and ceilings are not walls. true if it moved
bool pe_collision_mesh_push_out(const PCollisionMesh *mesh, vec3 centre,
                                float radius, const vec3 up);

#endif // !PE_TERRAIN_COLLISION_H
