#ifndef PE_SKY_H
#define PE_SKY_H

#include "camera.h"
#include "model.h"

//a sky from an equirectangular high dynamic range panorama, a Radiance .hdr. it
//is drawn first, over the whole target and under everything, so what is drawn
//after it goes over. the world is Z up and the middle of the panorama is toward
//+Y
typedef struct PSky {
  //what carries the uniform buffers and the descriptor sets, as a model does,
  //though it has no mesh
  PModel carrier;
  PShader shader;

  //what the light of the panorama is multiplied by before it is tonemapped
  float exposure;
} PSky;

//after pe_vk_init(). false when the file is missing or is not an .hdr
bool pe_sky_load(PSky *sky, const char *hdr_path);

void pe_sky_draw(PSky *sky, const PCamera *camera, VkCommandBuffer command,
                 u32 image_index);

#endif // !PE_SKY_H
