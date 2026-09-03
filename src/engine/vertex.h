#ifndef PE_VERTEX_H
#define PE_VERTEX_H

#include <cglm/cglm.h>

#include "numbers.h"

typedef struct PVertex {
  vec3 position;
  vec3 color;
  vec3 normal;
  vec2 uv;
  vec4 joint;
  vec4 weight;
  long unsigned int id;
  bool selected;
} PVertex;

//how many characters of a name an instance can carry, 4 packed per uint
#define PINSTANCE_NAME_MAX 32
#define PINSTANCE_NAME_WORDS (PINSTANCE_NAME_MAX / 4)

//one of these per copy of a mesh instead of per vertex, so a whole
//skyline can be drawn with a single vkCmdDrawIndexed
typedef struct PInstance {
  vec3 position;
  vec3 scale;
  vec3 color;
  u32 name[PINSTANCE_NAME_WORDS];
  float name_length;
} PInstance;

#endif // !PE_VERTEX_H
