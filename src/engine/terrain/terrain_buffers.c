#include "terrain_mesh.h"

#include <string.h>

void pe_vk_terrain_mesh_upload(const PTerrainMesh *mesh, PTerrainGpuMesh *gpu) {
  gpu->vertex_buffer = pe_vk_create_buffer(sizeof(mesh->vertices),
                                           (void *)mesh->vertices,
                                           VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
  gpu->index_buffer = pe_vk_create_buffer(
      mesh->index_count * sizeof(mesh->indices[0]), (void *)mesh->indices,
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

  memcpy(gpu->chunks, mesh->chunks, sizeof(gpu->chunks));
  memcpy(gpu->bounds, mesh->bounds, sizeof(gpu->bounds));
}
