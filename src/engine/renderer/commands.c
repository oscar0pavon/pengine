#include "commands.h"
#include <engine/macros.h>
#include "engine/camera.h"
#include "framebuffer.h"
#include <vulkan/vulkan_core.h>
#include "render_pass.h"
#include "vulkan.h"

VkCommandPool pe_vk_commands_pool;

VkCommandBuffer pe_vk_begin_single_time_cmd(){
    
    VkCommandBufferAllocateInfo bufferinfo;
    ZERO(bufferinfo);
    bufferinfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    bufferinfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    bufferinfo.commandPool = pe_vk_commands_pool;
    bufferinfo.commandBufferCount = 1;
    
    VkCommandBuffer command_buffer;
    ZERO(command_buffer);
    vkAllocateCommandBuffers(vk_device,&bufferinfo,&command_buffer);

    VkCommandBufferBeginInfo info;
    ZERO(info);
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(command_buffer,&info);

    return command_buffer;

}


void pe_vk_end_single_time_cmd(VkCommandBuffer buffer){
   vkEndCommandBuffer(buffer);

    VkSubmitInfo submitInfo;
    ZERO(submitInfo);
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &buffer;

    vkQueueSubmit(vk_queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(vk_queue);

    vkFreeCommandBuffers(vk_device, pe_vk_commands_pool, 1, &buffer);
}

void pe_vk_commands_pool_init(){

  VkCommandPoolCreateInfo info;
  ZERO(info);
  info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  info.queueFamilyIndex = q_graphic_family;

  vkCreateCommandPool(vk_device, &info, NULL, &pe_vk_commands_pool);

}

void pe_vk_target_command_pool_init(PRenderTarget *target){

  VkCommandPoolCreateInfo info;
  ZERO(info);
  info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  info.queueFamilyIndex = q_graphic_family;

  vkCreateCommandPool(vk_device, &info, NULL, &target->commands_pool);

}

VkCommandBuffer pe_vk_start_record_command(PRenderTarget *target,
                                           int swapchain_image_index) {

  VkCommandBufferBeginInfo begininfo = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = 0,
      .pInheritanceInfo = NULL

  };

  VkCommandBuffer *command =
      array_get(&target->command_buffers, swapchain_image_index);

  vkBeginCommandBuffer(*command, &begininfo);

  return *command;
}

void pe_vk_command_init(PRenderTarget *target) {


  array_init(&target->command_buffers, sizeof(VkCommandBuffer),
             target->framebuffers.count);
  array_resize(&target->command_buffers, target->framebuffers.count);

  VkCommandBufferAllocateInfo bufferinfo;
  ZERO(bufferinfo);
  bufferinfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  bufferinfo.commandPool = target->commands_pool;
  bufferinfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  bufferinfo.commandBufferCount = target->command_buffers.count;

  vkAllocateCommandBuffers(vk_device, &bufferinfo, target->command_buffers.data);

}

void pe_vk_clean_commands(){

  vkDestroyCommandPool(vk_device, pe_vk_commands_pool, NULL);

}


void pe_vk_end_command(VkCommandBuffer command) {
  vkEndCommandBuffer(command);
}
