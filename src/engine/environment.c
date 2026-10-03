#include "environment.h"

#include <engine/images.h>
#include <engine/macros.h>
#include <engine/renderer/descriptor_set.h>
#include <engine/renderer/vk_buffer.h>
#include <engine/renderer/vulkan.h>

#include <math.h>
#include <stdlib.h>

//every this many pixels of the panorama is looked at to find its light. the
//light changes slowly over a sky, and the sun's few pixels are found by the
//blocks they are the brightest of, not one by one
#define SAMPLE_STRIDE 4
#define SUN_BLOCK 32

#define SUN_WARMTH_R 1.0f
#define SUN_WARMTH_G 0.93f
#define SUN_WARMTH_B 0.82f

//where a pixel of the panorama looks, the same mapping the sky shader reads it by
static void pixel_direction(const PHdrImage *image, float x, float y, vec3 out) {
  float azimuth = (x / image->width - 0.5f) * 2 * GLM_PI;
  float elevation = (0.5f - y / image->height) * GLM_PI;

  out[0] = cosf(elevation) * sinf(azimuth);
  out[1] = cosf(elevation) * cosf(azimuth);
  out[2] = sinf(elevation);
}

static void sh_basis(const vec3 d, float out[9]) {
  out[0] = 0.282095f;
  out[1] = 0.488603f * d[1];
  out[2] = 0.488603f * d[2];
  out[3] = 0.488603f * d[0];
  out[4] = 1.092548f * d[0] * d[1];
  out[5] = 1.092548f * d[1] * d[2];
  out[6] = 0.315392f * (3 * d[2] * d[2] - 1);
  out[7] = 1.092548f * d[0] * d[2];
  out[8] = 0.546274f * (d[0] * d[0] - d[1] * d[1]);
}

//Ramamoorthi and Hanrahan's way to the light that reaches a surface: project
//the panorama onto nine spherical harmonics, and weigh each band by how much
//of it a cosine lobe keeps
static void project_irradiance(const PHdrImage *image, vec4 out[9]) {
  static const float band_weight[9] = {GLM_PI,
                                       2 * GLM_PI / 3, 2 * GLM_PI / 3, 2 * GLM_PI / 3,
                                       GLM_PI / 4, GLM_PI / 4, GLM_PI / 4,
                                       GLM_PI / 4, GLM_PI / 4};
  double sums[9][3] = {{0}};

  float pixel_angle = SAMPLE_STRIDE * GLM_PI / image->height;
  float solid_angle_scale = pixel_angle * (SAMPLE_STRIDE * 2 * GLM_PI / image->width);

  for (int y = 0; y < image->height; y += SAMPLE_STRIDE)
    for (int x = 0; x < image->width; x += SAMPLE_STRIDE) {
      vec3 direction;
      pixel_direction(image, x + SAMPLE_STRIDE * 0.5f, y + SAMPLE_STRIDE * 0.5f,
                      direction);

      float basis[9];
      sh_basis(direction, basis);
      float solid_angle = cosf((0.5f - (y + SAMPLE_STRIDE * 0.5f) / image->height) *
                               GLM_PI) * solid_angle_scale;

      const float *light = &image->pixels[((size_t)y * image->width + x) * 3];
      for (int i = 0; i < 9; i++)
        for (int c = 0; c < 3; c++)
          sums[i][c] += light[c] * basis[i] * solid_angle;
    }

  for (int i = 0; i < 9; i++) {
    for (int c = 0; c < 3; c++)
      out[i][c] = (float)sums[i][c] * band_weight[i] / GLM_PI;
    out[i][3] = 0;
  }
}

//toward the middle of the block of the panorama that holds the most light
static void brightest_direction(const PHdrImage *image, vec3 out) {
  double best = -1;
  int best_x = 0, best_y = 0;

  for (int block_y = 0; block_y + SUN_BLOCK <= image->height; block_y += SUN_BLOCK)
    for (int block_x = 0; block_x + SUN_BLOCK <= image->width; block_x += SUN_BLOCK) {
      double sum = 0;
      for (int y = block_y; y < block_y + SUN_BLOCK; y += 2)
        for (int x = block_x; x < block_x + SUN_BLOCK; x += 2) {
          const float *p = &image->pixels[((size_t)y * image->width + x) * 3];
          sum += p[0] + p[1] + p[2];
        }
      if (sum > best) {
        best = sum;
        best_x = block_x + SUN_BLOCK / 2;
        best_y = block_y + SUN_BLOCK / 2;
      }
    }

  pixel_direction(image, best_x, best_y, out);
}

static void create_set(PEnvironment *environment) {
  VkDescriptorPoolSize sizes[2] = {
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}};
  VkDescriptorPoolCreateInfo pool_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .poolSizeCount = 2,
      .pPoolSizes = sizes,
      .maxSets = 1};
  VKVALID(vkCreateDescriptorPool(vk_device, &pool_info, NULL, &environment->pool),
          "Can't create environment descriptor pool");

  VkDescriptorSetAllocateInfo alloc_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = environment->pool,
      .descriptorSetCount = 1,
      .pSetLayouts = &pe_vk_descriptor_set_layout_environment};
  VKVALID(vkAllocateDescriptorSets(vk_device, &alloc_info, &environment->set),
          "Can't allocate environment descriptor set");

  VkDescriptorBufferInfo buffer_info = {.buffer = environment->buffer.buffer,
                                        .offset = 0,
                                        .range = sizeof(PEnvironmentBlock)};
  VkDescriptorImageInfo image_info = {
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      .imageView = environment->panorama.image_view,
      .sampler = environment->panorama.sampler};
  VkWriteDescriptorSet writes[2] = {
      {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
       .dstSet = environment->set,
       .dstBinding = 0,
       .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
       .descriptorCount = 1,
       .pBufferInfo = &buffer_info},
      {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
       .dstSet = environment->set,
       .dstBinding = 1,
       .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
       .descriptorCount = 1,
       .pImageInfo = &image_info}};
  vkUpdateDescriptorSets(vk_device, 2, writes, 0, NULL);
}

bool pe_environment_load(PEnvironment *environment, const char *hdr_path,
                         float exposure, float sun_intensity) {
  ZERO(*environment);

  PHdrImage image;
  if (!pe_load_hdr_image(hdr_path, &image))
    return false;

  PEnvironmentBlock *block = &environment->block;
  project_irradiance(&image, block->irradiance);

  vec3 sun;
  brightest_direction(&image, sun);
  glm_vec4(sun, 0, block->sun_direction);
  glm_vec4_copy((vec4){SUN_WARMTH_R * sun_intensity, SUN_WARMTH_G * sun_intensity,
                       SUN_WARMTH_B * sun_intensity, 1},
                block->sun_color);

  pe_texture_from_hdr_image(&environment->panorama, &image);
  pe_free_hdr_image(&image);

  block->settings[0] = exposure;
  block->settings[1] = (float)environment->panorama.mip_level - 1;

  environment->buffer = pe_vk_create_buffer(sizeof(PEnvironmentBlock), block,
                                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
  create_set(environment);
  return true;
}
