#ifndef PE_ENVIRONMENT_H
#define PE_ENVIRONMENT_H

#include "model.h"

#include <cglm/cglm.h>

//the light of a world: a high dynamic range panorama that lights what is in it
//from every side and shows in what is shiny, and a sun. the world is Z up and
//the middle of the panorama is toward +Y
typedef struct PEnvironmentBlock {
  //the light that reaches a surface from the panorama, as nine spherical
  //harmonics already divided by pi, so the shader multiplies it by the albedo
  vec4 irradiance[9];

  //toward the sun, unit length, and its light
  vec4 sun_direction;
  vec4 sun_color;

  //exposure, the number of the panorama's mip levels less one
  vec4 settings;
} PEnvironmentBlock;

typedef struct PEnvironment {
  PTexture panorama;
  PBuffer buffer;
  VkDescriptorPool pool;

  //set 1 of pe_vk_pipeline_layout_pbr
  VkDescriptorSet set;

  PEnvironmentBlock block;
} PEnvironment;

//after pe_vk_init(). false when the file is missing or is not an .hdr. the sun
//is put toward the brightest part of the panorama, and is as bright as
//sun_intensity says
bool pe_environment_load(PEnvironment *environment, const char *hdr_path,
                         float exposure, float sun_intensity);

#endif // !PE_ENVIRONMENT_H
