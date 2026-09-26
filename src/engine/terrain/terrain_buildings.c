#include "terrain_buildings.h"
#include "terrain_frustum.h"

#include <engine/log.h>
#include <engine/macros.h>
#include <engine/renderer/vulkan.h>

#include <limits.h>
#include <stdio.h>
#include <string.h>

//INFO what is cut out and what is not: an opaque material keeps every pixel
//and an alpha tested one drops those below half. the buildings in the game's
//data use only those two
#define ALPHA_TEST_CUTOFF 0.5f

void pe_vk_terrain_buildings_create(PTerrainBuildings *buildings) {
  VkDescriptorPoolSize size = {
      .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = PE_TERRAIN_MATERIAL_SETS_MAX};
  VkDescriptorPoolCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = PE_TERRAIN_MATERIAL_SETS_MAX,
      .poolSizeCount = 1,
      .pPoolSizes = &size};

  VKVALID(vkCreateDescriptorPool(vk_device, &info, NULL, &buildings->pool),
          "Can't create building material descriptor pool");
}

static PTerrainGpuBuilding *find_building(PTerrainBuildings *buildings,
                                          const char *name) {
  for (u32 i = 0; i < buildings->building_count; i++)
    if (strcmp(buildings->buildings[i].name, name) == 0)
      return &buildings->buildings[i];
  return NULL;
}

static bool allocate_material_set(const PTerrainPipeline *pipeline,
                                  PTerrainBuildings *buildings,
                                  VkDescriptorSet *set) {
  if (buildings->sets_used == PE_TERRAIN_MATERIAL_SETS_MAX)
    return false;

  VkDescriptorSetAllocateInfo allocation = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = buildings->pool,
      .descriptorSetCount = 1,
      .pSetLayouts = &pipeline->building_material_layout};

  if (vkAllocateDescriptorSets(vk_device, &allocation, set) != VK_SUCCESS)
    return false;

  buildings->sets_used++;
  return true;
}

static void write_material_set(VkDescriptorSet set, const PTexture *texture) {
  VkDescriptorImageInfo image = {
      .sampler = texture->sampler,
      .imageView = texture->image_view,
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
  VkWriteDescriptorSet write = {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = set,
      .dstBinding = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .pImageInfo = &image};

  vkUpdateDescriptorSets(vk_device, 1, &write, 0, NULL);
}

static bool create_materials(const PTerrainPipeline *pipeline,
                             PTerrainTextures *textures,
                             PTerrainBuildings *buildings,
                             const PBuilding *source, const char *directory,
                             PTerrainGpuBuilding *gpu) {
  gpu->material_count = source->material_count;

  for (u32 i = 0; i < source->material_count; i++) {
    const PBuildingMaterial *material = &source->materials[i];

    const PTexture *texture =
        material->texture == PE_BUILDING_NO_TEXTURE
            ? pe_vk_terrain_texture_missing(textures)
            : pe_vk_terrain_texture_get(textures, directory,
                                        source->textures[material->texture]);

    if (allocate_material_set(pipeline, buildings, &gpu->material_sets[i]) ==
        false)
      return false;

    write_material_set(gpu->material_sets[i], texture);
    gpu->alpha_cutoffs[i] =
        material->blend == PE_BUILDING_BLEND_ALPHA_TEST ? ALPHA_TEST_CUTOFF : 0;
  }
  return true;
}

static PTerrainGpuBuilding *load_building(const PTerrainPipeline *pipeline,
                                          PTerrainTextures *textures,
                                          PTerrainBuildings *buildings,
                                          const char *name,
                                          const char *directory) {
  if (buildings->building_count == PE_TERRAIN_GPU_BUILDINGS_MAX) {
    LOG("terrain: no room for building %s\n", name);
    return NULL;
  }

  PTerrainGpuBuilding *gpu = &buildings->buildings[buildings->building_count++];
  ZERO(*gpu);
  strcpy(gpu->name, name);

  char path[PATH_MAX];
  snprintf(path, sizeof(path), "%s/%s", directory, name);

  PBuilding source;
  if (pe_building_load(path, &source) == false)
    return gpu;

  gpu->usable = create_materials(pipeline, textures, buildings, &source,
                                 directory, gpu);
  if (gpu->usable == false)
    LOG("terrain: no room for the materials of %s\n", name);

  if (gpu->usable) {
    gpu->vertex_buffer = pe_vk_create_buffer(
        source.vertex_count * sizeof(PBuildingVertex), source.vertices,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    gpu->index_buffer = pe_vk_create_buffer(source.index_count * sizeof(u32),
                                            source.indices,
                                            VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    gpu->batch_count = source.batch_count;
    gpu->batches = source.batches;
    source.batches = NULL;

    gpu->group_count = source.group_count;
    memcpy(gpu->groups, source.groups, sizeof(source.groups));
  }

  pe_building_free(&source);
  return gpu;
}

static bool is_already_placed(const PTerrainBuildings *buildings,
                              u32 unique_id) {
  if (unique_id == 0)
    return false;

  for (u32 i = 0; i < buildings->instance_count; i++)
    if (buildings->instances[i].unique_id == unique_id)
      return true;
  return false;
}

void pe_vk_terrain_buildings_add_tile(const PTerrainPipeline *pipeline,
                                      PTerrainTextures *textures,
                                      PTerrainBuildings *buildings,
                                      const PTerrainTile *tile,
                                      const char *directory) {
  for (u32 i = 0; i < tile->placement_count; i++) {
    const PTerrainPlacement *placement = &tile->placements[i];

    if (is_already_placed(buildings, placement->unique_id))
      continue;

    if (buildings->instance_count == PE_TERRAIN_INSTANCES_MAX) {
      LOG("terrain: no room for more than %d buildings\n",
          PE_TERRAIN_INSTANCES_MAX);
      return;
    }

    const char *name = tile->buildings[placement->building];
    PTerrainGpuBuilding *gpu = find_building(buildings, name);
    if (gpu == NULL)
      gpu = load_building(pipeline, textures, buildings, name, directory);
    if (gpu == NULL || gpu->usable == false)
      continue;

    PTerrainBuildingInstance *instance =
        &buildings->instances[buildings->instance_count++];
    instance->building = gpu - buildings->buildings;
    instance->unique_id = placement->unique_id;
    pe_terrain_placement_matrix(placement, instance->model);

    //the box the game stored for it is the box round all of it where it stands
    glm_vec3_center((float *)&placement->bounds[0],
                    (float *)&placement->bounds[3], instance->sphere);
    instance->sphere[3] = glm_vec3_distance((float *)&placement->bounds[0],
                                            (float *)&placement->bounds[3]) /
                          2;
  }
}

static void draw_batches(const PTerrainPipeline *pipeline,
                         const PTerrainGpuBuilding *building,
                         bool camera_in_a_room, VkCommandBuffer command) {
  u32 last_material = UINT32_MAX;
  float last_cutoff = -1;

  for (u32 i = 0; i < building->batch_count; i++) {
    const PBuildingBatch *batch = &building->batches[i];

    if (camera_in_a_room == false &&
        pe_building_group_is_room(&building->groups[batch->group]))
      continue;

    if (batch->material != last_material) {
      vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              pipeline->building_layout, 1, 1,
                              &building->material_sets[batch->material], 0,
                              NULL);
      last_material = batch->material;
    }

    float cutoff = building->alpha_cutoffs[batch->material];
    if (cutoff != last_cutoff) {
      vkCmdPushConstants(command, pipeline->building_layout,
                         VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(mat4),
                         sizeof(cutoff), &cutoff);
      last_cutoff = cutoff;
    }

    vkCmdDrawIndexed(command, batch->index_count, 1, batch->first_index, 0, 0);
  }
}

static void bind_building(const PTerrainGpuBuilding *building,
                          VkCommandBuffer command) {
  VkDeviceSize no_offset = 0;

  vkCmdBindVertexBuffers(command, 0, 1, &building->vertex_buffer.buffer,
                         &no_offset);
  vkCmdBindIndexBuffer(command, building->index_buffer.buffer, 0,
                       VK_INDEX_TYPE_UINT32);
}

u32 pe_vk_terrain_buildings_draw(const PTerrainPipeline *pipeline,
                                 const PTerrainFrames *frames,
                                 const PTerrainBuildings *buildings,
                                 const PTerrainFrame *frame,
                                 VkCommandBuffer command, u32 image_index) {
  if (buildings->instance_count == 0)
    return 0;

  mat4 view_projection;
  glm_mat4_mul((vec4 *)frame->projection, (vec4 *)frame->view, view_projection);
  vec4 planes[PE_TERRAIN_FRUSTUM_PLANES];
  pe_terrain_frustum_planes(view_projection, planes);
  float view_distance = frame->fog_range[1];

  vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipeline->building.pipeline);
  vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipeline->building_layout, 0, 1,
                          &frames->sets[image_index], 0, NULL);

  u32 drawn = 0;

  //one building at a time, so its buffers are bound once for every placement
  //of it rather than once for each
  for (u32 b = 0; b < buildings->building_count; b++) {
    const PTerrainGpuBuilding *building = &buildings->buildings[b];
    bool bound = false;

    for (u32 i = 0; i < buildings->instance_count; i++) {
      const PTerrainBuildingInstance *instance = &buildings->instances[i];

      if (instance->building != b ||
          pe_terrain_sphere_in_frustum(planes, instance->sphere) == false)
        continue;

      if (view_distance > 0 &&
          pe_terrain_sphere_within(instance->sphere, frame->camera_position,
                                   view_distance) == false)
        continue;

      if (bound == false) {
        bind_building(building, command);
        bound = true;
      }

      //the camera in the building's own axes, which is where its rooms are
      mat4 inverse;
      glm_mat4_inv((vec4 *)instance->model, inverse);
      vec4 local;
      glm_mat4_mulv(inverse, (float *)frame->camera_position, local);

      vkCmdPushConstants(command, pipeline->building_layout,
                         VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4),
                         instance->model);
      draw_batches(pipeline, building,
                   pe_building_camera_in_a_room(building->groups,
                                                building->group_count, local),
                   command);
      drawn++;
    }
  }
  return drawn;
}
