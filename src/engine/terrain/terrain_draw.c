#include "terrain_draw.h"

#include <engine/renderer/vulkan.h>

#include <time.h>

#define FRUSTUM_PLANES 6

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

static void matrix_row(const mat4 matrix, int row, vec4 out) {
  for (int i = 0; i < 4; i++)
    out[i] = matrix[i][row];
}

//INFO the planes come from the rows of the projection times the view. the
//depth range is 0 to 1 here, so the near plane is the third row alone, where
//-1 to 1 would make it the fourth row plus the third. each plane points into
//the frustum, so a point inside has a positive distance from all six
static void frustum_planes(const mat4 view_projection,
                           vec4 planes[FRUSTUM_PLANES]) {
  vec4 x, y, z, w;
  matrix_row(view_projection, 0, x);
  matrix_row(view_projection, 1, y);
  matrix_row(view_projection, 2, z);
  matrix_row(view_projection, 3, w);

  glm_vec4_add(w, x, planes[0]);
  glm_vec4_sub(w, x, planes[1]);
  glm_vec4_add(w, y, planes[2]);
  glm_vec4_sub(w, y, planes[3]);
  glm_vec4_copy(z, planes[4]);
  glm_vec4_sub(w, z, planes[5]);

  for (int i = 0; i < FRUSTUM_PLANES; i++)
    glm_vec4_scale(planes[i], 1 / glm_vec3_norm(planes[i]), planes[i]);
}

static bool sphere_is_visible(vec4 planes[FRUSTUM_PLANES],
                              const vec4 sphere) {
  for (int i = 0; i < FRUSTUM_PLANES; i++)
    if (glm_vec3_dot(planes[i], (float *)sphere) + planes[i][3] < -sphere[3])
      return false;
  return true;
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

static bool sphere_is_within(const vec4 sphere, const vec4 point,
                             float distance) {
  return glm_vec3_distance((float *)sphere, (float *)point) - sphere[3] <=
         distance;
}

u32 pe_vk_terrain_draw(const PTerrainDrawInfo *draw) {
  VkCommandBuffer command = draw->command_buffer;
  VkDeviceSize no_offset = 0;
  u32 drawn = 0;
  float view_distance = draw->frame->fog_range[1];

  mat4 view_projection;
  glm_mat4_mul((vec4 *)draw->frame->projection, (vec4 *)draw->frame->view,
               view_projection);
  vec4 planes[FRUSTUM_PLANES];
  frustum_planes(view_projection, planes);

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
        sphere_is_visible(planes, draw->mesh->bounds[chunk]) == false)
      continue;

    if (view_distance > 0 &&
        sphere_is_within(draw->mesh->bounds[chunk],
                         draw->frame->camera_position, view_distance) == false)
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
