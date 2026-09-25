#ifndef PE_TERRAIN_MESH_H
#define PE_TERRAIN_MESH_H

#include "terrain.h"

#include <cglm/cglm.h>
#include <engine/renderer/vk_buffer.h>

#define PE_TERRAIN_TILE_VERTICES (PE_TERRAIN_CHUNKS * PE_TERRAIN_CHUNK_VERTICES)

//8x8 quads, each drawn as four triangles fanned from its centre vertex
#define PE_TERRAIN_CHUNK_INDICES_MAX (8 * 8 * 4 * 3)
#define PE_TERRAIN_TILE_INDICES_MAX                                            \
  (PE_TERRAIN_CHUNKS * PE_TERRAIN_CHUNK_INDICES_MAX)

typedef struct PTerrainVertex {
  vec3 position;
  vec3 normal;

  //what the base and layer textures are sampled with, continuous across chunks
  vec2 uv;

  //where in the chunk's alpha maps this vertex falls
  vec2 layer_uv;
} PTerrainVertex;

_Static_assert(sizeof(PTerrainVertex) == 10 * sizeof(float),
               "the pipeline reads this as ten tightly packed floats");

typedef struct PTerrainChunkRange {
  u32 first_index;
  u32 index_count;
} PTerrainChunkRange;

//about 2.3 MB. a chunk's indices are relative to the tile's vertex array, not
//to the chunk, so one vertex buffer and one index buffer serve all 256 draws
typedef struct PTerrainMesh {
  PTerrainVertex vertices[PE_TERRAIN_TILE_VERTICES];
  u32 indices[PE_TERRAIN_TILE_INDICES_MAX];
  PTerrainChunkRange chunks[PE_TERRAIN_CHUNKS];

  //the sphere around each chunk: centre in xyz, radius in w
  vec4 bounds[PE_TERRAIN_CHUNKS];
  u32 index_count;
} PTerrainMesh;

typedef struct PTerrainGpuMesh {
  PBuffer vertex_buffer;
  PBuffer index_buffer;
  PTerrainChunkRange chunks[PE_TERRAIN_CHUNKS];
  vec4 bounds[PE_TERRAIN_CHUNKS];
} PTerrainGpuMesh;

void pe_terrain_mesh_build(const PTerrainTile *tile, PTerrainMesh *mesh);

//needs the renderer up, so from the game's init or later
void pe_vk_terrain_mesh_upload(const PTerrainMesh *mesh, PTerrainGpuMesh *gpu);

#endif // !PE_TERRAIN_MESH_H
