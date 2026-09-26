#ifndef PE_TERRAIN_PIPELINE_H
#define PE_TERRAIN_PIPELINE_H

#include "terrain.h"

#include <cglm/cglm.h>
#include <engine/renderer/shaders.h>
#include <engine/renderer/swap_chain.h>
#include <engine/renderer/vk_buffer.h>

//what terrain_frame.glsl declares, in its order. every terrain tile drawn in a
//frame reads the same one
typedef struct PTerrainFrame {
  mat4 view;
  mat4 projection;

  //the way the light travels, so straight down is 0, 0, -1
  vec4 light_direction;
  vec4 light_color;
  vec4 ambient_color;
  vec4 camera_position;
  vec4 fog_color;

  //x is the distance fog starts at and y the distance where nothing else shows
  vec4 fog_range;

  //the sky is the fog colour at the horizon and this straight up
  vec4 sky_zenith;

  //takes a point on the screen back to the world, for the sky's view direction
  mat4 inverse_view_projection;
} PTerrainFrame;

//set 0 is the frame, set 1 is one chunk's textures
typedef struct PTerrainPipeline {
  VkDescriptorSetLayout frame_layout;
  VkDescriptorSetLayout material_layout;
  VkPipelineLayout layout;
  PShader shader;

  //drawn first, over the whole screen, with the frame set alone
  PShader sky;
} PTerrainPipeline;

//one uniform buffer and descriptor set per swap chain image, so writing the
//next frame's never touches what the gpu is still reading
typedef struct PTerrainFrames {
  VkDescriptorPool pool;
  u32 count;
  PBuffer buffers[PE_VK_MAX_SWAPCHAIN_IMAGES];
  VkDescriptorSet sets[PE_VK_MAX_SWAPCHAIN_IMAGES];
} PTerrainFrames;

//all three need the renderer up, so from the game's init or later
void pe_vk_terrain_pipeline_create(PTerrainPipeline *pipeline);

void pe_vk_terrain_frames_create(const PTerrainPipeline *pipeline,
                                 PTerrainFrames *frames);

void pe_vk_terrain_frame_update(PTerrainFrames *frames, u32 image_index,
                                const PTerrainFrame *frame);

#endif // !PE_TERRAIN_PIPELINE_H
