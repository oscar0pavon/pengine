#ifndef PRENDERER_MATERIAL
#define PRENDERER_MATERIAL

#include <cglm/cglm.h>

typedef struct PMaterial{
  int shader;
  vec4 color;   
}PMaterial;

#endif
