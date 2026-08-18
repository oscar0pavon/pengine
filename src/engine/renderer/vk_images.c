
#include "vk_images.h"
#include "commands.h"
#include "engine/images.h"
#include "images_view.h"
#include "direct_memory_access.h"
#include "swap_chain.h"
#include "vk_buffer.h"
#include "vk_memory.h"
#include "vulkan.h"
#include <cglm/cglm.h>
#include <engine/log.h>
#include <engine/macros.h>
#include <math.h>
#include <stdalign.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <vulkan/vulkan_core.h>
#include <wchar.h>
#include <wctype.h>

PTexture vk_depth_image;

VkImageView pe_vk_depth_image_view;

VkImageMemoryBarrier pe_vk_create_barrier() {

  VkImageMemoryBarrier barrier = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .subresourceRange.baseMipLevel = 0,
      .subresourceRange.levelCount = 1,
      .subresourceRange.baseArrayLayer = 0,
      .subresourceRange.layerCount = 1};

  return barrier;
}

void pe_vk_image_to_destination(VkImage image) {

  VkCommandBuffer command = pe_vk_begin_single_time_cmd();

  VkImageMemoryBarrier barrier = pe_vk_create_barrier();
  barrier.image = image;
  barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.srcAccessMask = 0;
  barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;


  VkPipelineStageFlags source_stage, destination_stage;
  source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;

  vkCmdPipelineBarrier(command, source_stage, destination_stage, 0, 0, NULL, 0,
                       NULL, 1, &barrier);

  pe_vk_end_single_time_cmd(command);
}

void pe_vk_image_transfer_to_present(VkImage image) {

  VkCommandBuffer command = pe_vk_begin_single_time_cmd();

  VkImageMemoryBarrier barrier = pe_vk_create_barrier();
  barrier.image = image;
  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  barrier.newLayout= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;


  VkPipelineStageFlags source_stage, destination_stage;
  source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  destination_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

  vkCmdPipelineBarrier(command, source_stage, destination_stage, 0, 0, NULL, 0,
                       NULL, 1, &barrier);

  pe_vk_end_single_time_cmd(command);
}

void pe_vk_image_color_to_transfer(VkImage image) {

  VkCommandBuffer command = pe_vk_begin_single_time_cmd();

  VkImageMemoryBarrier barrier = pe_vk_create_barrier();
  barrier.image = image;
  barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;


  VkPipelineStageFlags source_stage, destination_stage;
  source_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;

  vkCmdPipelineBarrier(command, source_stage, destination_stage, 0, 0, NULL, 0,
                       NULL, 1, &barrier);

  pe_vk_end_single_time_cmd(command);
}
void pe_vk_image_to_color_attacthment(VkImage image) {

  VkCommandBuffer command = pe_vk_begin_single_time_cmd();

  VkImageMemoryBarrier barrier = pe_vk_create_barrier();
  barrier.image = image;
  barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  barrier.srcAccessMask = 0;
  barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;


  VkPipelineStageFlags source_stage, destination_stage;
  source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  destination_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

  vkCmdPipelineBarrier(command, source_stage, destination_stage, 0, 0, NULL, 0,
                       NULL, 1, &barrier);

  pe_vk_end_single_time_cmd(command);
}

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

void pe_vk_copy_image(VkImage source, VkImage destination) {

  VkCommandBuffer command = pe_vk_begin_single_time_cmd();

  VkImageCopy copy_info = {
      .srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .srcSubresource.baseArrayLayer = 0,
      .srcSubresource.layerCount = 1,
      .srcSubresource.mipLevel = 0,
      .srcOffset = {0, 0, 0},
      .dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .dstSubresource.baseArrayLayer = 0,
      .dstSubresource.layerCount = 1,
      .dstSubresource.mipLevel = 0,
      .dstOffset = {0, 0, 0},
      .extent = {1920, 1080, 1} // Swapchain image dimensions
  };

  vkCmdCopyImage(command, source, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                 destination, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                 &copy_info);

  pe_vk_end_single_time_cmd(command);
}

void pe_vk_handle_error(VkResult result) {

  if (result == VK_ERROR_OUT_OF_HOST_MEMORY) {
    fprintf(stderr, "Error: Host memory exhausted.\n");
  } else if (result == VK_ERROR_OUT_OF_DEVICE_MEMORY) {
    fprintf(stderr, "Error: GPU memory exhausted.\n");
  } else if (result == VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR) {
    fprintf(stderr,
            "Error: Invalid FD provided by client, handle is invalid.\n");
  }
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
  if(info->texture->mip_level == 0){
    info->texture->mip_level = 1;
  }

  VkExternalMemoryImageCreateInfo external_info = {};
  VkMemoryDedicatedAllocateInfo dedicate_info = {};
  if(info->is_exportable || info->is_importable){
      external_info.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
      external_info.pNext = info->pNext;
      external_info.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;

  }
  
  VkImageCreateInfo image_info = {.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                                  .imageType = VK_IMAGE_TYPE_2D,
                                  .extent.width = info->width,
                                  .extent.height = info->height,
                                  .extent.depth = 1,
                                  .mipLevels = info->texture->mip_level,
                                  .arrayLayers = 1,
                                  .format = info->format,
                                  .tiling = info->tiling,
                                  .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                                  .usage = info->usage,
                                  .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                                  .samples = info->number_of_samples};

  if(info->is_exportable || info->is_importable){
    image_info.pNext = &external_info;
  }

  VKVALID(vkCreateImage(vk_device, &image_info, NULL, &info->texture->image),
          "failed to create image !");

  VkMemoryRequirements memory_requirements;
  vkGetImageMemoryRequirements(vk_device, info->texture->image,
                               &memory_requirements);

  VkExportMemoryAllocateInfo export_memory_info = {};
  if(info->is_exportable){
    export_memory_info.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
    export_memory_info.pNext = NULL;
    export_memory_info.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;

  }

  //printf("Memory rquirement size %li \n",memory_requirements.size);
  VkMemoryAllocateInfo info_alloc = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memory_requirements.size,
      .memoryTypeIndex = pe_vk_memory_find_type(
          memory_requirements.memoryTypeBits, info->properties)};

  if(info->is_exportable){
    info_alloc.pNext = &export_memory_info;
  }

  if(info->is_importable){
    printf("File descriptor %i\n", info->import.file_descriptor);
    info->import.info.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR;
    info->import.info.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;
    info->import.info.fd = info->import.file_descriptor;
    info->import.info.pNext = NULL;

    
    dedicate_info.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
    dedicate_info.pNext = &info->import.info;
    dedicate_info.image = info->texture->image;
    dedicate_info.buffer = NULL;
    
    info_alloc.pNext = &dedicate_info;
  }

  VkResult memory_allocation_result;

  memory_allocation_result =
      vkAllocateMemory(vk_device, &info_alloc, NULL, &info->texture->memory);

  //printf("Vulkan memory allocated: %p\n",info->texture->memory);
 
  if (memory_allocation_result != VK_SUCCESS) {
    printf("Can't allocate memory for image\n");
    pe_vk_handle_error(memory_allocation_result);
    sleep(2);
    exit(-1);
  }

  vkBindImageMemory(vk_device, info->texture->image, info->texture->memory, 0);

 
  if(info->is_exportable){

    VkMemoryGetFdInfoKHR file_descriptor_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR,
        .memory = info->texture->memory,
        .pNext = &export_memory_info, 
        .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT};


    VKVALID(pe_vk_get_memory_file_descriptor(vk_device, &file_descriptor_info,
                             &info->texture->memory_file_descriptor),
            "Can't get DMA file descriptor");
  }

}

void pe_vk_create_texture_sampler(PTexture* new_texture) {
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
  samplerInfo.maxLod = new_texture->mip_level;
  samplerInfo.anisotropyEnable = VK_FALSE;
  samplerInfo.maxAnisotropy = 1.0f;

  vkCreateSampler(vk_device, &samplerInfo, NULL, &new_texture->sampler);
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
      .texture = &vk_depth_image,
      .format = format,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
      .properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      .mip_level = 1,
      .number_of_samples = pe_vk_msaa_samples};

  pe_vk_create_image(&image_create_info);

  pe_vk_depth_image_view = pe_vk_create_image_view(
      vk_depth_image.image, format, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

}

void pe_vk_clean_image(PTexture* image){
  
  vkDestroyImage(vk_device, image->image, NULL);
  vkDestroyImageView(vk_device, image->image_view, NULL);
  if(image->sampler != VK_NULL_HANDLE) {
    vkDestroySampler(vk_device, image->sampler, NULL);
  }

  vkFreeMemory(vk_device, image->memory, NULL);
}

void pe_vk_create_exportable_images(){
  for (int i = 0; i < pe_vk_swapchain_image_count; i++) {
    PImageCreateInfo image_create_info = {
        .width = 1920,
        .height = 1080,
        .texture = &pe_vk_exportable_images[i],
        //.format = VK_FORMAT_R8G8B8A8_SRGB,//TODO could be this
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .number_of_samples = VK_SAMPLE_COUNT_1_BIT};
    
    pe_vk_create_image(&image_create_info);

  }
}

void pe_vk_import_image(PTexture *new_texture, uint32_t witdh, uint32_t height,
                        uint32_t file_descriptor, uint64_t modifier) {

  VkFormat format = VK_FORMAT_B8G8R8A8_UNORM;

  //format = find_compatible_external_format(vk_physical_device);

  new_texture->mip_level = 1;
  VkImageDrmFormatModifierListCreateInfoEXT modifier_list_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_LIST_CREATE_INFO_EXT,
      .pNext = NULL,
      .drmFormatModifierCount = 1,
      .pDrmFormatModifiers = &modifier,
  };

  PImageCreateInfo image_create_info = {
      .is_exportable = false,
      .is_importable = true,
      .width = witdh,
      .height = height,
      .texture = new_texture,
      .format = format,
      .tiling = VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT,
      .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
      .properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      .number_of_samples = VK_SAMPLE_COUNT_1_BIT,
      .import.file_descriptor = file_descriptor, 
      .pNext = &modifier_list_info};

  pe_vk_create_image(&image_create_info);

  pe_vk_create_texture_sampler(new_texture);

  new_texture->image_view = pe_vk_create_image_view(new_texture->image, format,
                                                    VK_IMAGE_ASPECT_COLOR_BIT,
                                                    new_texture->mip_level);
}

void pe_vk_create_texture(PTexture* new_texture, const char* path) {
  PImage texture;
  ZERO(texture);
  pe_load_image(path, &texture);

  // new_texture->mip_level =
  //     floor(log2(GLM_MAX(texture.width, texture.heigth))) + 1;
  new_texture->mip_level = 1;

  VkDeviceSize image_size = texture.width * texture.heigth * 4;

  PBuffer image_buffer = pe_vk_create_buffer(image_size, texture.pixels_data,
                                              VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

  PImageCreateInfo image_create_info = {
      .is_exportable = false, 
      .width = texture.width,
      .height = texture.heigth,
      .texture = new_texture,
      .format = VK_FORMAT_R8G8B8A8_SRGB,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
               VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
      .properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      .number_of_samples = VK_SAMPLE_COUNT_1_BIT};

  pe_vk_create_image(&image_create_info);

  pe_vk_transition_image_layout(
      new_texture->image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, new_texture->mip_level);

  pe_vk_image_copy_buffer(image_buffer.buffer, new_texture->image, texture.width,
                          texture.heigth);

  // pe_vk_transition_image_layout(pe_vk_texture_image, VK_FORMAT_R8G8B8A8_SRGB,
  //                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
  //                               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
  //                               pe_vk_mip_levels);

  pe_vk_image_generate_mipmaps(new_texture->image, texture.width,
                               texture.heigth, new_texture->mip_level);

  new_texture->image_view = pe_vk_create_image_view(
      new_texture->image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT,
      new_texture->mip_level);

  pe_vk_create_texture_sampler(new_texture);

  vkFreeMemory(vk_device, image_buffer.memory, NULL); // Clean the stagin buffer

  free_image(&texture);
}
