#ifndef PE_SKY_H
#define PE_SKY_H

#include "camera.h"
#include "environment.h"
#include "model.h"

//the sky of an environment: its panorama, drawn first, over the whole target and
//under everything, so what is drawn after it goes over
typedef struct PSky {
  //what carries the uniform buffers and the descriptor sets, as a model does,
  //though it has no mesh
  PModel carrier;
  PShader shader;

  float exposure;
} PSky;

//after pe_vk_init() and pe_environment_load(). the panorama is the
//environment's, shared, not a copy
void pe_sky_create(PSky *sky, const PEnvironment *environment);

void pe_sky_draw(PSky *sky, const PCamera *camera, VkCommandBuffer command,
                 u32 image_index);

#endif // !PE_SKY_H
