#ifndef PE_TERRAIN_DRAW_H
#define PE_TERRAIN_DRAW_H

#include "terrain_materials.h"
#include "terrain_mesh.h"
#include "terrain_pipeline.h"

#include <engine/camera.h>

typedef struct PTerrainDrawInfo {
  const PTerrainPipeline *pipeline;
  const PTerrainFrames *frames;
  const PTerrainGpuMesh *mesh;
  const PTerrainMaterials *materials;

  //what the chunks are tested against. it has to be the same one that was
  //sent to the frame's uniform buffer, or what is culled is not what is drawn
  const PTerrainFrame *frame;

  VkCommandBuffer command_buffer;
  u32 image_index;
} PTerrainDrawInfo;

//takes the view, the projection and the camera position, and leaves the
//lighting and the fog for the application to set
void pe_terrain_frame_set_camera(PTerrainFrame *frame, const PCamera *camera);

//the sky over the whole screen. it has to come before anything that is drawn
//over it
void pe_vk_terrain_sky_draw(const PTerrainPipeline *pipeline,
                            const PTerrainFrames *frames,
                            VkCommandBuffer command, u32 image_index);

//records one draw per chunk the camera can see and returns how many. call it
//from the pe_vk_draw_scene hook, after pe_vk_terrain_frame_update()
u32 pe_vk_terrain_draw(const PTerrainDrawInfo *draw);

#endif // !PE_TERRAIN_DRAW_H
