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
      .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
      .maxSets = PE_TERRAIN_MATERIAL_SETS_MAX,
      .poolSizeCount = 1,
      .pPoolSizes = &size};

  VKVALID(vkCreateDescriptorPool(vk_device, &info, NULL, &buildings->pool),
          "Can't create building material descriptor pool");
}

static PTerrainGpuBuilding *find_building(PTerrainBuildings *buildings,
                                          const char *name) {
  for (u32 i = 0; i < buildings->building_count; i++)
    if (buildings->buildings[i].in_use &&
        strcmp(buildings->buildings[i].name, name) == 0)
      return &buildings->buildings[i];
  return NULL;
}

static PTerrainGpuBuilding *take_slot(PTerrainBuildings *buildings) {
  for (u32 i = 0; i < buildings->building_count; i++)
    if (buildings->buildings[i].in_use == false)
      return &buildings->buildings[i];

  if (buildings->building_count == PE_TERRAIN_GPU_BUILDINGS_MAX)
    return NULL;
  return &buildings->buildings[buildings->building_count++];
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

static PBuildingShader material_shader(const PBuildingMaterial *material) {
  if (material->blend == PE_BUILDING_BLEND_ALPHA)
    return PE_BUILDING_SHADER_ALPHA;
  if (material->blend == PE_BUILDING_BLEND_ADD)
    return PE_BUILDING_SHADER_ADD;

  return material->flags & PE_BUILDING_MATERIAL_TWO_SIDED
             ? PE_BUILDING_SHADER_TWO_SIDED
             : PE_BUILDING_SHADER_ONE_SIDED;
}

//material_count is how many are made, so that what is left of a building that
//ran out of room can be given back
static bool create_materials(const PTerrainPipeline *pipeline,
                             PTerrainTextures *textures,
                             PTerrainBuildings *buildings,
                             const PBuilding *source, const char *directory,
                             PTerrainGpuBuilding *gpu) {
  for (u32 i = 0; i < source->material_count; i++) {
    const PBuildingMaterial *material = &source->materials[i];

    if (allocate_material_set(pipeline, buildings, &gpu->material_sets[i]) ==
        false)
      return false;
    gpu->material_count++;

    const PTexture *texture =
        material->texture == PE_BUILDING_NO_TEXTURE
            ? pe_vk_terrain_texture_missing(textures)
            : pe_vk_terrain_texture_get(textures, directory,
                                        source->textures[material->texture]);
    gpu->material_textures[i] = texture;

    write_material_set(gpu->material_sets[i], texture);
    gpu->alpha_cutoffs[i] =
        material->blend == PE_BUILDING_BLEND_ALPHA_TEST ? ALPHA_TEST_CUTOFF : 0;
    gpu->shaders[i] = material_shader(material);
    gpu->has_blended |= gpu->shaders[i] >= PE_BUILDING_SHADER_ALPHA;
  }
  return true;
}

//the props inside a building are in the .wwd beside its .wwb
static void load_doodads(const char *name, const char *directory,
                         PTerrainGpuBuilding *gpu) {
  char path[PATH_MAX];
  snprintf(path, sizeof(path), "%s/%.*s.wwd", directory,
           (int)strlen(name) - 4, name);

  pe_building_doodads_load(path, &gpu->doodads);
}

//a building is walked into by the triangles it is drawn with, and a prop by the
//few of its own in the .wwc beside its .wwb, if it has any
static void load_collision(const PBuilding *source, const char *name,
                           const char *directory, bool as_drawn,
                           PCollisionMesh *mesh) {
  if (as_drawn) {
    pe_collision_mesh_from_building(source, mesh);
    return;
  }

  char path[PATH_MAX];
  snprintf(path, sizeof(path), "%s/%.*s.wwc", directory, (int)strlen(name) - 4,
           name);
  pe_collision_mesh_load(path, mesh);
}

static void free_materials(PTerrainBuildings *buildings,
                           PTerrainTextures *textures,
                           PTerrainGpuBuilding *gpu) {
  if (gpu->material_count == 0)
    return;

  vkFreeDescriptorSets(vk_device, buildings->pool, gpu->material_count,
                       gpu->material_sets);
  buildings->sets_used -= gpu->material_count;

  for (u32 i = 0; i < gpu->material_count; i++)
    pe_vk_terrain_texture_release(textures, gpu->material_textures[i]);
  gpu->material_count = 0;
}

static void free_building(PTerrainBuildings *buildings,
                          PTerrainTextures *textures,
                          PTerrainGpuBuilding *gpu) {
  free_materials(buildings, textures, gpu);
  pe_vk_destroy_buffer(&gpu->vertex_buffer);
  pe_vk_destroy_buffer(&gpu->index_buffer);
  free(gpu->batches);
  pe_collision_mesh_free(&gpu->collision);
  pe_building_doodads_free(&gpu->doodads);
  ZERO(*gpu);
}

static PTerrainGpuBuilding *load_building(const PTerrainPipeline *pipeline,
                                          PTerrainTextures *textures,
                                          PTerrainBuildings *buildings,
                                          const char *name,
                                          const char *directory,
                                          bool collision_as_drawn) {
  PTerrainGpuBuilding *gpu = take_slot(buildings);
  if (gpu == NULL) {
    LOG("terrain: no room for building %s\n", name);
    return NULL;
  }

  ZERO(*gpu);
  gpu->in_use = true;
  strcpy(gpu->name, name);

  char path[PATH_MAX];
  snprintf(path, sizeof(path), "%s/%s", directory, name);

  PBuilding source;
  if (pe_building_load(path, &source) == false)
    return gpu;

  gpu->usable = create_materials(pipeline, textures, buildings, &source,
                                 directory, gpu);
  if (gpu->usable == false) {
    LOG("terrain: no room for the materials of %s\n", name);
    free_materials(buildings, textures, gpu);
  }

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

  if (gpu->usable)
    load_collision(&source, name, directory, collision_as_drawn,
                   &gpu->collision);

  pe_building_free(&source);
  if (gpu->usable)
    load_doodads(name, directory, gpu);
  return gpu;
}

static bool is_owned_by(const PTerrainBuildingInstance *instance, u16 owner) {
  for (u32 i = 0; i < instance->owner_count; i++)
    if (instance->owners[i] == owner)
      return true;
  return false;
}

//a placement that is already in the world, because another tile listed it
//first, is not added again but stands for this tile too, so it is still there
//when the first tile is gone. false for one that is not in the world
static bool share_placement(PTerrainBuildings *buildings, u32 unique_id,
                            u16 owner) {
  bool found = false;

  if (unique_id == 0)
    return false;

  for (u32 i = 0; i < buildings->instance_count; i++) {
    PTerrainBuildingInstance *instance = &buildings->instances[i];

    if (instance->unique_id != unique_id)
      continue;

    found = true;
    if (is_owned_by(instance, owner) == false &&
        instance->owner_count < PE_TERRAIN_INSTANCE_OWNERS_MAX)
      instance->owners[instance->owner_count++] = owner;
  }
  return found;
}

static u16 tile_owner(int tile_x, int tile_y) {
  return tile_y * PE_TERRAIN_TILES_PER_SIDE + tile_x;
}

//how much bigger than its model an instance is drawn
static float instance_scale(const PTerrainBuildingInstance *instance) {
  return glm_vec3_norm((float *)instance->model[0]);
}

//the sphere round a building where it stands: the one round its model, moved
//and grown as it is
static void place_sphere(const PTerrainGpuBuilding *building,
                         PTerrainBuildingInstance *instance) {
  glm_mat4_mulv3(instance->model, (float *)building->sphere, 1,
                 instance->sphere);
  instance->sphere[3] = building->sphere[3] * instance_scale(instance);
}

static bool add_instance(PTerrainBuildings *buildings,
                         const PTerrainGpuBuilding *gpu, u32 unique_id,
                         u16 owner, const mat4 model) {
  if (buildings->instance_count == PE_TERRAIN_INSTANCES_MAX) {
    LOG("terrain: no room for more than %d buildings and props\n",
        PE_TERRAIN_INSTANCES_MAX);
    return false;
  }

  PTerrainBuildingInstance *instance =
      &buildings->instances[buildings->instance_count++];
  instance->building = gpu - buildings->buildings;
  instance->unique_id = unique_id;
  instance->owners[0] = owner;
  instance->owner_count = 1;
  glm_mat4_copy((vec4 *)model, instance->model);
  place_sphere(gpu, instance);
  return true;
}

static PTerrainGpuBuilding *usable_building(const PTerrainPipeline *pipeline,
                                            PTerrainTextures *textures,
                                            PTerrainBuildings *buildings,
                                            const char *name,
                                            const char *directory,
                                            bool collision_as_drawn) {
  PTerrainGpuBuilding *gpu = find_building(buildings, name);
  if (gpu == NULL)
    gpu = load_building(pipeline, textures, buildings, name, directory,
                        collision_as_drawn);
  return gpu != NULL && gpu->usable ? gpu : NULL;
}

//the props inside a building, of set 0 and of the set this placement chose. they
//have no unique id of their own, they carry the building's, and it stops a second
//copy and gives them the same owners
static bool add_doodad_set(const PTerrainPipeline *pipeline,
                           PTerrainTextures *textures,
                           PTerrainBuildings *buildings, const char *directory,
                           const PTerrainGpuBuilding *building, u32 set,
                           u32 unique_id, u16 owner, const mat4 model) {
  const PBuildingDoodads *doodads = &building->doodads;
  if (set >= doodads->set_count)
    return true;

  for (u32 i = 0; i < doodads->sets[set].count; i++) {
    const PBuildingDoodad *doodad =
        &doodads->items[doodads->sets[set].first + i];

    PTerrainGpuBuilding *gpu = usable_building(
        pipeline, textures, buildings, doodads->models[doodad->model],
        directory, false);
    if (gpu == NULL)
      continue;

    mat4 placed;
    pe_building_doodad_matrix(model, doodad, placed);
    if (add_instance(buildings, gpu, unique_id, owner, placed) == false)
      return false;
  }
  return true;
}

static bool add_doodads(const PTerrainPipeline *pipeline,
                        PTerrainTextures *textures, PTerrainBuildings *buildings,
                        const char *directory,
                        const PTerrainGpuBuilding *building,
                        const PTerrainPlacement *placement, u16 owner,
                        const mat4 model) {
  if (add_doodad_set(pipeline, textures, buildings, directory, building, 0,
                     placement->unique_id, owner, model) == false)
    return false;

  return placement->doodad_set == 0 ||
         add_doodad_set(pipeline, textures, buildings, directory, building,
                        placement->doodad_set, placement->unique_id, owner,
                        model);
}

static void add_placements(const PTerrainPipeline *pipeline,
                           PTerrainTextures *textures,
                           PTerrainBuildings *buildings, const char *directory,
                           const char (*names)[PE_TERRAIN_BUILDING_PATH_MAX],
                           const PTerrainPlacement *placements, u32 count,
                           u16 owner, bool collision_as_drawn) {
  for (u32 i = 0; i < count; i++) {
    const PTerrainPlacement *placement = &placements[i];

    if (share_placement(buildings, placement->unique_id, owner))
      continue;

    PTerrainGpuBuilding *gpu = usable_building(
        pipeline, textures, buildings, names[placement->model], directory,
        collision_as_drawn);
    if (gpu == NULL)
      continue;

    mat4 model;
    pe_terrain_placement_matrix(placement, model);
    if (add_instance(buildings, gpu, placement->unique_id, owner, model) ==
            false ||
        add_doodads(pipeline, textures, buildings, directory, gpu, placement,
                    owner, model) == false)
      return;
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
  u16 owner = tile_owner(tile->tile_x, tile->tile_y);

  add_placements(pipeline, textures, buildings, directory,
                 (const char(*)[PE_TERRAIN_BUILDING_PATH_MAX])tile->buildings,
                 tile->placements, tile->placement_count, owner, true);
  add_placements(pipeline, textures, buildings, directory,
                 (const char(*)[PE_TERRAIN_BUILDING_PATH_MAX])tile->props,
                 tile->prop_placements, tile->prop_placement_count, owner,
                 false);

  qsort(buildings->instances, buildings->instance_count,
        sizeof(buildings->instances[0]), compare_buildings);
}

static void disown(PTerrainBuildingInstance *instance, u16 owner) {
  for (u32 i = 0; i < instance->owner_count; i++) {
    if (instance->owners[i] != owner)
      continue;

    instance->owners[i] = instance->owners[--instance->owner_count];
    return;
  }
}

//one that could not be loaded holds nothing, and is kept so it is not tried
//again by every tile that places it
static void free_unplaced_buildings(PTerrainBuildings *buildings,
                                    PTerrainTextures *textures) {
  static bool placed[PE_TERRAIN_GPU_BUILDINGS_MAX];
  memset(placed, 0, sizeof(placed));

  for (u32 i = 0; i < buildings->instance_count; i++)
    placed[buildings->instances[i].building] = true;

  for (u32 i = 0; i < buildings->building_count; i++) {
    PTerrainGpuBuilding *gpu = &buildings->buildings[i];

    if (gpu->in_use && gpu->usable && placed[i] == false)
      free_building(buildings, textures, gpu);
  }
}

void pe_vk_terrain_buildings_remove_tile(PTerrainBuildings *buildings,
                                         PTerrainTextures *textures,
                                         int tile_x, int tile_y) {
  u16 owner = tile_owner(tile_x, tile_y);
  u32 kept = 0;

  for (u32 i = 0; i < buildings->instance_count; i++) {
    PTerrainBuildingInstance *instance = &buildings->instances[i];

    disown(instance, owner);
    if (instance->owner_count > 0)
      buildings->instances[kept++] = *instance;
  }

  buildings->instance_count = kept;
  free_unplaced_buildings(buildings, textures);
}

//what the fragment shader is told about a material: where it cuts out, and 0
//if it is solid, 1 if it is blended over what is behind, 2 if added to it
typedef struct MaterialConstants {
  float alpha_cutoff;
  float blend;
} MaterialConstants;

//draws the batches of one building that are blended, or all those that are
//solid. bound is which pipeline is bound, and is left as it is at the end, so
//the next building need not bind it again
static void draw_batches(const PTerrainPipeline *pipeline,
                         const PTerrainGpuBuilding *building, bool blended,
                         PBuildingShader *bound, VkCommandBuffer command) {
  u32 last_material = UINT32_MAX;

  for (u32 i = 0; i < building->batch_count; i++) {
    const PBuildingBatch *batch = &building->batches[i];
    PBuildingShader shader = building->shaders[batch->material];

    if ((shader >= PE_BUILDING_SHADER_ALPHA) != blended)
      continue;

    if (shader != *bound) {
      vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS,
                        pipeline->building_shaders[shader].pipeline);
      *bound = shader;
    }

    if (batch->material != last_material) {
      MaterialConstants constants = {
          .alpha_cutoff = building->alpha_cutoffs[batch->material],
          .blend = blended ? shader - PE_BUILDING_SHADER_TWO_SIDED : 0};

      vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              pipeline->building_layout, 1, 1,
                              &building->material_sets[batch->material], 0,
                              NULL);
      vkCmdPushConstants(command, pipeline->building_layout,
                         VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(mat4),
                         sizeof(constants), &constants);
      last_material = batch->material;
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

static bool is_visible(const PTerrainBuildingInstance *instance,
                       vec4 *planes, const PTerrainFrame *frame) {
  float view_distance = frame->fog_range[1];

  return pe_terrain_sphere_in_frustum(planes, instance->sphere) &&
         (view_distance <= 0 ||
          pe_terrain_sphere_within(instance->sphere, frame->camera_position,
                                   view_distance));
}

static void push_model(const PTerrainPipeline *pipeline,
                       const PTerrainBuildingInstance *instance,
                       VkCommandBuffer command) {
  vkCmdPushConstants(command, pipeline->building_layout,
                     VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4),
                     instance->model);
}

static int farther_first(const void *left, const void *right) {
  const PTerrainBlendedInstance *a = left;
  const PTerrainBlendedInstance *b = right;

  return (a->distance < b->distance) - (a->distance > b->distance);
}

//what is solid first, remembering which of what it drew has something blended
//in it. returns how many were drawn
static u32 draw_solid(const PTerrainPipeline *pipeline,
                      PTerrainBuildings *buildings, vec4 *planes,
                      const PTerrainFrame *frame, PBuildingShader *bound,
                      VkCommandBuffer command) {
  u32 drawn = 0;
  u32 bound_building = UINT32_MAX;
  buildings->blended_count = 0;

  for (u32 i = 0; i < buildings->instance_count; i++) {
    const PTerrainBuildingInstance *instance = &buildings->instances[i];
    const PTerrainGpuBuilding *building =
        &buildings->buildings[instance->building];

    if (is_visible(instance, planes, frame) == false)
      continue;

    if (instance->building != bound_building) {
      bind_building(building, command);
      bound_building = instance->building;
    }

    push_model(pipeline, instance, command);
    draw_batches(pipeline, building, false, bound, command);
    drawn++;

    if (building->has_blended)
      buildings->blended[buildings->blended_count++] =
          (PTerrainBlendedInstance){
              .instance = i,
              .distance = glm_vec3_distance2(
                  (float *)instance->sphere, (float *)frame->camera_position)};
  }
  return drawn;
}

static void draw_blended(const PTerrainPipeline *pipeline,
                         PTerrainBuildings *buildings, PBuildingShader *bound,
                         VkCommandBuffer command) {
  u32 bound_building = UINT32_MAX;

  qsort(buildings->blended, buildings->blended_count,
        sizeof(buildings->blended[0]), farther_first);

  for (u32 i = 0; i < buildings->blended_count; i++) {
    const PTerrainBuildingInstance *instance =
        &buildings->instances[buildings->blended[i].instance];
    const PTerrainGpuBuilding *building =
        &buildings->buildings[instance->building];

    if (instance->building != bound_building) {
      bind_building(building, command);
      bound_building = instance->building;
    }

    push_model(pipeline, instance, command);
    draw_batches(pipeline, building, true, bound, command);
  }
}

u32 pe_vk_terrain_buildings_draw(const PTerrainPipeline *pipeline,
                                 const PTerrainFrames *frames,
                                 PTerrainBuildings *buildings,
                                 const PTerrainFrame *frame,
                                 VkCommandBuffer command, u32 image_index) {
  if (buildings->instance_count == 0)
    return 0;

  mat4 view_projection;
  glm_mat4_mul((vec4 *)frame->projection, (vec4 *)frame->view, view_projection);
  vec4 planes[PE_TERRAIN_FRUSTUM_PLANES];
  pe_terrain_frustum_planes(view_projection, planes);

  PBuildingShader bound = PE_BUILDING_SHADER_ONE_SIDED;
  vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipeline->building_shaders[bound].pipeline);
  vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipeline->building_layout, 0, 1,
                          &frames->sets[image_index], 0, NULL);

  u32 drawn = draw_solid(pipeline, buildings, planes, frame, &bound, command);
  draw_blended(pipeline, buildings, &bound, command);
  return drawn;
}

static bool reaches(const PTerrainBuildingInstance *instance, const vec3 point,
                    float reach) {
  return glm_vec3_distance((float *)instance->sphere, (float *)point) <=
         instance->sphere[3] + reach;
}

bool pe_terrain_buildings_floor_at(const PTerrainBuildings *buildings,
                                   const vec3 from, float *height) {
  bool found = false;
  *height = -1e30f;

  for (u32 i = 0; i < buildings->instance_count; i++) {
    const PTerrainBuildingInstance *instance = &buildings->instances[i];
    const PCollisionMesh *mesh = &buildings->buildings[instance->building].collision;

    if (mesh->triangle_count == 0 ||
        reaches(instance, from, PE_TERRAIN_FLOOR_REACH) == false)
      continue;

    mat4 inverse;
    glm_mat4_inv((vec4 *)instance->model, inverse);

    //the way down in the model's own axes, which are tilted if it is, and its
    //own units, which are the world's divided by its scale
    float scale = instance_scale(instance);
    vec3 origin, down;
    glm_mat4_mulv3(inverse, (float *)from, 1, origin);
    glm_mat4_mulv3(inverse, (vec3){0, 0, -1}, 0, down);
    glm_vec3_normalize(down);

    float distance;
    if (pe_collision_mesh_floor(mesh, origin, down,
                                PE_TERRAIN_FLOOR_REACH / scale, &distance) &&
        from[2] - distance * scale > *height) {
      *height = from[2] - distance * scale;
      found = true;
    }
  }
  return found;
}

bool pe_terrain_buildings_push_out(const PTerrainBuildings *buildings,
                                   vec3 centre, float radius) {
  bool moved = false;

  for (u32 i = 0; i < buildings->instance_count; i++) {
    const PTerrainBuildingInstance *instance = &buildings->instances[i];
    const PCollisionMesh *mesh = &buildings->buildings[instance->building].collision;

    if (mesh->triangle_count == 0 || reaches(instance, centre, radius) == false)
      continue;

    mat4 inverse;
    glm_mat4_inv((vec4 *)instance->model, inverse);

    vec3 local, up;
    glm_mat4_mulv3(inverse, centre, 1, local);
    glm_mat4_mulv3(inverse, (vec3){0, 0, 1}, 0, up);
    glm_vec3_normalize(up);

    vec3 before;
    glm_vec3_copy(local, before);
    if (pe_collision_mesh_push_out(mesh, local,
                                   radius / instance_scale(instance),
                                   up) == false)
      continue;

    vec3 push;
    glm_vec3_sub(local, before, push);
    glm_mat4_mulv3((vec4 *)instance->model, push, 0, push);
    glm_vec3_add(centre, push, centre);
    moved = true;
  }
  return moved;
}
