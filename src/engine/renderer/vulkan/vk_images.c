
#include "vk_images.h"
#include "commands.h"
#include "engine/images.h"
#include "images_view.h"
#include "swap_chain.h"
#include "vk_buffer.h"
#include "vk_memory.h"
#include "vulkan.h"
#include <ThirdParty/cglm/cglm.h>
#include <engine/log.h>
#include <engine/macros.h>
#include <math.h>
#include <stdalign.h>
#include <stdint.h>
#include <vulkan/vulkan_core.h>
#include <wchar.h>
#include <wctype.h>

#include <engine/files.h>

void pe_vk_transition_image_layout(VkImage image, VkFormat format,
                                   VkImageLayout old_layout,
                                   VkImageLayout new_layout,
                                   uint32_t mip_level) {
  VkCommandBuffer command = pe_vk_begin_single_time_cmd();

  VkImageMemoryBarrier barrier = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .oldLayout = old_layout,
      .newLayout = new_layout,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = image,
      .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .subresourceRange.baseMipLevel = 0,
      .subresourceRange.levelCount = mip_level,
      .subresourceRange.baseArrayLayer = 0,
      .subresourceRange.layerCount = 1};

  VkPipelineStageFlags source_stage;
  ZERO(source_stage);
  VkPipelineStageFlags destination_stage;
  ZERO(destination_stage);

  if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
      new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;

  } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else {
    LOG("Unsupported layout transition");
  }

  vkCmdPipelineBarrier(command, source_stage, destination_stage, 0, 0, NULL, 0,
                       NULL, 1, &barrier);

  pe_vk_end_single_time_cmd(command);
}

void pe_vk_image_copy_buffer(VkBuffer buffer, VkImage image, uint32_t width,
                             uint32_t height) {
  VkCommandBuffer command = pe_vk_begin_single_time_cmd();

  VkBufferImageCopy region = {.bufferOffset = 0,
                              .bufferRowLength = 0,
                              .bufferImageHeight = 0,
                              .imageSubresource.aspectMask =
                                  VK_IMAGE_ASPECT_COLOR_BIT,
                              .imageSubresource.mipLevel = 0,
                              .imageSubresource.baseArrayLayer = 0,
                              .imageSubresource.layerCount = 1,
                              .imageOffset = {0, 0, 0},
                              .imageExtent = {width, height, 1}};

  vkCmdCopyBufferToImage(command, buffer, image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  pe_vk_end_single_time_cmd(command);
}

void pe_vk_create_image(PImageCreateInfo *info) {

  VkImageCreateInfo imageInfo;
  ZERO(imageInfo);

  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = info->width;
  imageInfo.extent.height = info->height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = info->mip_level;
  imageInfo.arrayLayers = 1;
  imageInfo.format = info->format;
  imageInfo.tiling = info->tiling;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = info->usage;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.samples = info->number_of_samples;
  imageInfo.flags = 0; // Optional

  if (vkCreateImage(vk_device, &imageInfo, NULL, info->texture_image) !=
      VK_SUCCESS) {
    LOG("failed to create image!\n");
  }

  VkMemoryRequirements image_memory_requirements;
  vkGetImageMemoryRequirements(vk_device, *(info->texture_image),
                               &image_memory_requirements);

  VkMemoryAllocateInfo info_alloc;
  ZERO(info_alloc);
  info_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  info_alloc.allocationSize = image_memory_requirements.size;
  info_alloc.memoryTypeIndex = pe_vk_memory_find_type(
      image_memory_requirements.memoryTypeBits, info->properties);

  vkAllocateMemory(vk_device, &info_alloc, NULL, info->image_memory);

  vkBindImageMemory(vk_device, *(info->texture_image), *(info->image_memory),
                    0);
}

void pe_vk_create_texture_sampler() {
  VkSamplerCreateInfo samplerInfo = {};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = pe_vk_mip_levels;
  samplerInfo.anisotropyEnable = VK_FALSE;
  samplerInfo.maxAnisotropy = 1.0f;

  vkCreateSampler(vk_device, &samplerInfo, NULL, &pe_vk_texture_sampler);
}

void pe_vk_image_generate_mipmaps(VkImage image, uint32_t width,
                                  uint32_t heigth, uint32_t mip_levels) {
  VkCommandBuffer command = pe_vk_begin_single_time_cmd();

  VkImageMemoryBarrier barrier = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .image = image,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .subresourceRange.baseArrayLayer = 0,
      .subresourceRange.layerCount = 1,
      .subresourceRange.levelCount = 1};

  int32_t mip_width = width;
  int32_t mip_heigth = heigth;

  for (uint32_t i = 1; i < mip_levels; i++) {
    barrier.subresourceRange.baseMipLevel = i - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1,
                         &barrier);

    VkImageBlit blit = {};
    VkOffset3D src_offset1 = {0, 0, 0};
    VkOffset3D src_offset2 = {mip_width, mip_heigth, 1};

    VkOffset3D dst_offset1 = {0, 0, 0};
    VkOffset3D dst_offset2 = {mip_heigth > 1 ? mip_width / 2 : 1,
                              mip_width > 1 ? mip_heigth / 2 : 1, 1};

    blit.srcOffsets[0] = src_offset1;
    blit.srcOffsets[1] = src_offset2;
    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.mipLevel = i - 1;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount = 1;
    blit.dstOffsets[0] = dst_offset1;
    blit.dstOffsets[1] = dst_offset2;
    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.mipLevel = i;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount = 1;

    vkCmdBlitImage(command, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
                   VK_FILTER_LINEAR);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0,
                         NULL, 1, &barrier);

    if (mip_width > 1)
      mip_width /= 2;
    if (mip_heigth > 1)
      mip_heigth /= 2;
  }

  barrier.subresourceRange.baseMipLevel = mip_levels - 1;
  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0,
                       NULL, 1, &barrier);

  pe_vk_end_single_time_cmd(command);
}

void pe_vk_create_depth_resources() {
  VkFormat format = VK_FORMAT_D32_SFLOAT;

  PImageCreateInfo image_create_info = {
      .width = pe_vk_swch_extent.width,
      .height = pe_vk_swch_extent.height,
      .texture_image = &pe_vk_depth_image,
      .image_memory = &pe_vk_depth_image_memory,
      .format = format,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
      .properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      .mip_level = 1,
      .number_of_samples = pe_vk_msaa_samples};

  pe_vk_create_image(&image_create_info);

  pe_vk_depth_image_view = pe_vk_create_image_view(
      pe_vk_depth_image, format, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
}

void pe_vk_create_texture_image() {
  PTexture texture;
  ZERO(texture);
  pe_load_texture(file_rotate_png, &texture);

  pe_vk_mip_levels =
      floor(log2(GLM_MAX(texture.image.width, texture.image.heigth))) + 1;

  LOG("Mip map level = %i\n", pe_vk_mip_levels);

  VkDeviceSize image_size = texture.image.width * texture.image.heigth * 4;

  PBufferCreateInfo buffer_info;
  ZERO(buffer_info);
  buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  buffer_info.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  buffer_info.size = image_size;
  pe_vk_buffer_create(&buffer_info);
  if (texture.image.pixels_data == NULL) {
    return;
  }
  void *data;
  VKVALID(vkMapMemory(vk_device, buffer_info.buffer_memory, 0, image_size, 0,
                      &data),
          "Can't map memory");
  memcpy(data, texture.image.pixels_data, image_size);
  vkUnmapMemory(vk_device, buffer_info.buffer_memory);

  PImageCreateInfo image_create_info = {
      .width = texture.image.width,
      .height = texture.image.heigth,
      .texture_image = &pe_vk_texture_image,
      .image_memory = &pe_vk_texture_image_memory,
      .format = VK_FORMAT_R8G8B8A8_SRGB,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
               VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
      .properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      .mip_level = pe_vk_mip_levels,
      .number_of_samples = VK_SAMPLE_COUNT_1_BIT};

  pe_vk_create_image(&image_create_info);

  pe_vk_transition_image_layout(
      pe_vk_texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, pe_vk_mip_levels);

  pe_vk_image_copy_buffer(buffer_info.buffer, pe_vk_texture_image,
                          texture.image.width, texture.image.heigth);

  // pe_vk_transition_image_layout(pe_vk_texture_image, VK_FORMAT_R8G8B8A8_SRGB,
  //                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
  //                               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
  //                               pe_vk_mip_levels);

  pe_vk_image_generate_mipmaps(pe_vk_texture_image, texture.image.width,
                               texture.image.heigth, pe_vk_mip_levels);

  pe_vk_texture_image_view =
      pe_vk_create_image_view(pe_vk_texture_image, VK_FORMAT_R8G8B8A8_SRGB,
                              VK_IMAGE_ASPECT_COLOR_BIT, pe_vk_mip_levels);

  pe_vk_create_texture_sampler();
}
