#ifndef PE_TERRAIN_WATER_H
#define PE_TERRAIN_WATER_H

#include "terrain_pipeline.h"

#include <cglm/cglm.h>

typedef struct PTerrainWaterVertex {
  vec3 position;

  //0 at the shore to 1 in the deepest water
  float depth;

  //a PTerrainLiquid, as a float so it goes through the pipeline untouched
  float type;
} PTerrainWaterVertex;

_Static_assert(sizeof(PTerrainWaterVertex) == 5 * sizeof(float),
               "the pipeline reads this as five tightly packed floats");

//all of a tile's water in one vertex buffer and one index buffer. a tile with
//none has an index_count of 0 and no buffers
typedef struct PTerrainGpuWater {
  PBuffer vertex_buffer;
  PBuffer index_buffer;
  u32 index_count;
} PTerrainGpuWater;

//needs the renderer up, so from the game's init or later
void pe_vk_terrain_water_upload(const PTerrainTile *tile,
                                PTerrainGpuWater *water);

//call it after the ground of every tile, since it blends over what is there
void pe_vk_terrain_water_draw(const PTerrainPipeline *pipeline,
                              const PTerrainFrames *frames,
                              const PTerrainGpuWater *water,
                              VkCommandBuffer command, u32 image_index);

#endif // !PE_TERRAIN_WATER_H
