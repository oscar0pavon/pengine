#include "terrain_materials.h"

#include <engine/macros.h>
#include <engine/renderer/vk_images.h>
#include <engine/renderer/vulkan.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#define ALPHA_ATLAS_DIM (PE_TERRAIN_CHUNKS_PER_SIDE * PE_TERRAIN_ALPHA_DIM)
#define ALPHA_CHANNELS (PE_TERRAIN_LAYERS_MAX - 1)

static void create_fallback(PTexture *texture) {
  u8 pixels[2 * 2 * 4] = {255, 0, 255, 255, 0,   0, 0,   255,
                          0,   0, 0,   255, 255, 0, 255, 255};
  PImage image = {.width = 2, .heigth = 2, .pixels_data = pixels};

  pe_vk_create_texture_from_image(texture, &image);
}

static void load_texture(PTexture *texture, const char *directory,
                         const char *name, const PTexture *fallback) {
  char path[PATH_MAX];
  PImage image;
  ZERO(image);

  snprintf(path, sizeof(path), "%s/%s", directory, name);

  if (pe_load_image(path, &image) == -1) {
    LOG("terrain: can't load texture %s\n", path);
    *texture = *fallback;
    return;
  }

  pe_vk_create_texture_from_image(texture, &image);
  free_image(&image);
}

//the atlas is 16 by 16 cells of one chunk's map each, in chunk order. the
//three layers above the base are the red, green and blue channels
static void create_alpha_atlas(const PTerrainTile *tile, PTexture *atlas) {
  u8 *pixels = malloc(ALPHA_ATLAS_DIM * ALPHA_ATLAS_DIM * 4);

  for (int chunk = 0; chunk < PE_TERRAIN_CHUNKS; chunk++) {
    int cell_x = (chunk % PE_TERRAIN_CHUNKS_PER_SIDE) * PE_TERRAIN_ALPHA_DIM;
    int cell_y = (chunk / PE_TERRAIN_CHUNKS_PER_SIDE) * PE_TERRAIN_ALPHA_DIM;

    for (int y = 0; y < PE_TERRAIN_ALPHA_DIM; y++) {
      for (int x = 0; x < PE_TERRAIN_ALPHA_DIM; x++) {
        u8 *texel = pixels + ((cell_y + y) * ALPHA_ATLAS_DIM + cell_x + x) * 4;

        for (int channel = 0; channel < ALPHA_CHANNELS; channel++)
          texel[channel] =
              tile->chunks[chunk]
                  .layer_alpha[channel][y * PE_TERRAIN_ALPHA_DIM + x];
        texel[3] = 255;
      }
    }
  }

  PImage image = {.width = ALPHA_ATLAS_DIM,
                  .heigth = ALPHA_ATLAS_DIM,
                  .pixels_data = pixels};
  pe_vk_create_texture_from_image_format(atlas, &image,
                                         VK_FORMAT_R8G8B8A8_UNORM, false);
  free(pixels);
}

//a slot the chunk has no layer for shows its base texture, and the alpha map
//has nothing in that channel to blend it in with
static const PTexture *layer_texture(const PTerrainMaterials *materials,
                                     const PTerrainChunk *chunk, u32 layer) {
  if (chunk->layer_count == 0)
    return &materials->fallback;

  u32 used = layer < chunk->layer_count ? layer : 0;
  return &materials->textures[chunk->layer_textures[used]];
}

static VkDescriptorPool create_pool(void) {
  VkDescriptorPoolSize size = {
      .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = PE_TERRAIN_CHUNKS * (PE_TERRAIN_LAYERS_MAX + 1)};
  VkDescriptorPoolCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = PE_TERRAIN_CHUNKS,
      .poolSizeCount = 1,
      .pPoolSizes = &size};

  VkDescriptorPool pool;
  VKVALID(vkCreateDescriptorPool(vk_device, &info, NULL, &pool),
          "Can't create terrain material descriptor pool");
  return pool;
}

static VkDescriptorImageInfo image_info(const PTexture *texture) {
  VkDescriptorImageInfo info = {
      .sampler = texture->sampler,
      .imageView = texture->image_view,
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
  return info;
}

static void write_chunk_set(const PTerrainMaterials *materials,
                            const PTerrainChunk *chunk, VkDescriptorSet set) {
  VkDescriptorImageInfo layers[PE_TERRAIN_LAYERS_MAX];
  for (u32 layer = 0; layer < PE_TERRAIN_LAYERS_MAX; layer++)
    layers[layer] = image_info(layer_texture(materials, chunk, layer));

  VkDescriptorImageInfo atlas = image_info(&materials->alpha_atlas);

  VkWriteDescriptorSet writes[] = {
      {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
       .dstSet = set,
       .dstBinding = 0,
       .descriptorCount = PE_TERRAIN_LAYERS_MAX,
       .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
       .pImageInfo = layers},
      {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
       .dstSet = set,
       .dstBinding = 1,
       .descriptorCount = 1,
       .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
       .pImageInfo = &atlas}};
  vkUpdateDescriptorSets(vk_device, 2, writes, 0, NULL);
}

void pe_vk_terrain_materials_create(const PTerrainPipeline *pipeline,
                                    const PTerrainTile *tile,
                                    const char *texture_directory,
                                    PTerrainMaterials *materials) {
  create_fallback(&materials->fallback);

  for (u32 i = 0; i < tile->texture_count; i++)
    load_texture(&materials->textures[i], texture_directory, tile->textures[i],
                 &materials->fallback);

  create_alpha_atlas(tile, &materials->alpha_atlas);

  VkDescriptorSetLayout layouts[PE_TERRAIN_CHUNKS];
  for (int i = 0; i < PE_TERRAIN_CHUNKS; i++)
    layouts[i] = pipeline->material_layout;

  materials->pool = create_pool();
  VkDescriptorSetAllocateInfo allocation = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = materials->pool,
      .descriptorSetCount = PE_TERRAIN_CHUNKS,
      .pSetLayouts = layouts};
  VKVALID(vkAllocateDescriptorSets(vk_device, &allocation,
                                   materials->chunk_sets),
          "Can't allocate terrain material descriptor sets");

  for (int i = 0; i < PE_TERRAIN_CHUNKS; i++)
    write_chunk_set(materials, &tile->chunks[i], materials->chunk_sets[i]);
}
