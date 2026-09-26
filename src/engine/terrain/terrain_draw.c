#include "terrain_draw.h"
#include "terrain_frustum.h"

#include <engine/renderer/vulkan.h>

#include <time.h>

void pe_terrain_frame_set_camera(PTerrainFrame *frame, const PCamera *camera) {
  glm_mat4_copy((vec4 *)camera->view, frame->view);
  glm_mat4_copy((vec4 *)camera->projection, frame->projection);
  glm_vec4((float *)camera->position, 1, frame->camera_position);

  mat4 view_projection;
  glm_mat4_mul(frame->projection, frame->view, view_projection);
  glm_mat4_inv(view_projection, frame->inverse_view_projection);

  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  frame->time[0] = (float)(now.tv_sec % 3600) + now.tv_nsec / 1e9f;
}

void pe_vk_terrain_sky_draw(const PTerrainPipeline *pipeline,
                            const PTerrainFrames *frames,
                            VkCommandBuffer command, u32 image_index) {
  vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipeline->sky.pipeline);
  vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipeline->layout, 0, 1, &frames->sets[image_index], 0,
                          NULL);
  vkCmdDraw(command, 3, 1, 0, 0);
}

u32 pe_vk_terrain_draw(const PTerrainDrawInfo *draw) {
  VkCommandBuffer command = draw->command_buffer;
  VkDeviceSize no_offset = 0;
  u32 drawn = 0;
  float view_distance = draw->frame->fog_range[1];

  mat4 view_projection;
  glm_mat4_mul((vec4 *)draw->frame->projection, (vec4 *)draw->frame->view,
               view_projection);
  vec4 planes[PE_TERRAIN_FRUSTUM_PLANES];
  pe_terrain_frustum_planes(view_projection, planes);

  vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    draw->pipeline->shader.pipeline);
  vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          draw->pipeline->layout, 0, 1,
                          &draw->frames->sets[draw->image_index], 0, NULL);
  vkCmdBindVertexBuffers(command, 0, 1, &draw->mesh->vertex_buffer.buffer,
                         &no_offset);
  vkCmdBindIndexBuffer(command, draw->mesh->index_buffer.buffer, 0,
                       VK_INDEX_TYPE_UINT32);

  for (u32 chunk = 0; chunk < PE_TERRAIN_CHUNKS; chunk++) {
    const PTerrainChunkRange *range = &draw->mesh->chunks[chunk];

    if (range->index_count == 0 ||
        pe_terrain_sphere_in_frustum(planes, draw->mesh->bounds[chunk]) == false)
      continue;

    if (view_distance > 0 &&
        pe_terrain_sphere_within(draw->mesh->bounds[chunk],
                                 draw->frame->camera_position,
                                 view_distance) == false)
      continue;

    vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            draw->pipeline->layout, 1, 1,
                            &draw->materials->chunk_sets[chunk], 0, NULL);
    //INFO firstInstance is the chunk number: the vertex shader finds the
    //chunk's alpha map in the atlas from it
    vkCmdDrawIndexed(command, range->index_count, 1, range->first_index, 0,
                     chunk);
    drawn++;
  }
  return drawn;
}
