#include "terrain_buildings.h"
#include "terrain_frustum.h"

#include <engine/log.h>
#include <engine/macros.h>
#include <engine/renderer/vulkan.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
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
    gpu->two_sided[i] = material->flags & PE_BUILDING_MATERIAL_TWO_SIDED;
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

    glm_vec3_center(&source.bounds[0], &source.bounds[3], gpu->sphere);
    gpu->sphere[3] =
        glm_vec3_distance(&source.bounds[0], &source.bounds[3]) / 2;
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

//the sphere round a building where it stands: the one round its model, moved
//and grown as it is
static void place_sphere(const PTerrainGpuBuilding *building,
                         const PTerrainPlacement *placement,
                         PTerrainBuildingInstance *instance) {
  glm_mat4_mulv3(instance->model, (float *)building->sphere, 1,
                 instance->sphere);
  instance->sphere[3] = building->sphere[3] * placement->scale;
}

static void add_placements(const PTerrainPipeline *pipeline,
                           PTerrainTextures *textures,
                           PTerrainBuildings *buildings, const char *directory,
                           const char (*names)[PE_TERRAIN_BUILDING_PATH_MAX],
                           const PTerrainPlacement *placements, u32 count) {
  for (u32 i = 0; i < count; i++) {
    const PTerrainPlacement *placement = &placements[i];

    if (is_already_placed(buildings, placement->unique_id))
      continue;

    if (buildings->instance_count == PE_TERRAIN_INSTANCES_MAX) {
      LOG("terrain: no room for more than %d buildings and props\n",
          PE_TERRAIN_INSTANCES_MAX);
      return;
    }

    const char *name = names[placement->model];
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
    place_sphere(gpu, placement, instance);
  }
}

static int compare_buildings(const void *left, const void *right) {
  const PTerrainBuildingInstance *a = left;
  const PTerrainBuildingInstance *b = right;

  return (a->building > b->building) - (a->building < b->building);
}

void pe_vk_terrain_buildings_add_tile(const PTerrainPipeline *pipeline,
                                      PTerrainTextures *textures,
                                      PTerrainBuildings *buildings,
                                      const PTerrainTile *tile,
                                      const char *directory) {
  add_placements(pipeline, textures, buildings, directory,
                 (const char(*)[PE_TERRAIN_BUILDING_PATH_MAX])tile->buildings,
                 tile->placements, tile->placement_count);
  add_placements(pipeline, textures, buildings, directory,
                 (const char(*)[PE_TERRAIN_BUILDING_PATH_MAX])tile->props,
                 tile->prop_placements, tile->prop_placement_count);

  qsort(buildings->instances, buildings->instance_count,
        sizeof(buildings->instances[0]), compare_buildings);
}

//two_sided is which of the two pipelines is bound, and is left as it is at the
//end so the next building need not bind it again
static void draw_batches(const PTerrainPipeline *pipeline,
                         const PTerrainGpuBuilding *building,
                         bool *two_sided, VkCommandBuffer command) {
  u32 last_material = UINT32_MAX;
  float last_cutoff = -1;

  for (u32 i = 0; i < building->batch_count; i++) {
    const PBuildingBatch *batch = &building->batches[i];

    if (building->two_sided[batch->material] != *two_sided) {
      *two_sided = building->two_sided[batch->material];
      vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS,
                        *two_sided ? pipeline->building_two_sided.pipeline
                                   : pipeline->building.pipeline);
    }

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
  u32 bound = UINT32_MAX;
  bool two_sided = false;

  for (u32 i = 0; i < buildings->instance_count; i++) {
    const PTerrainBuildingInstance *instance = &buildings->instances[i];
    const PTerrainGpuBuilding *building =
        &buildings->buildings[instance->building];

    if (pe_terrain_sphere_in_frustum(planes, instance->sphere) == false)
      continue;

    if (view_distance > 0 &&
        pe_terrain_sphere_within(instance->sphere, frame->camera_position,
                                 view_distance) == false)
      continue;

    if (instance->building != bound) {
      bind_building(building, command);
      bound = instance->building;
    }

    vkCmdPushConstants(command, pipeline->building_layout,
                       VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4),
                       instance->model);
    draw_batches(pipeline, building, &two_sided, command);
    drawn++;
  }
  return drawn;
}
