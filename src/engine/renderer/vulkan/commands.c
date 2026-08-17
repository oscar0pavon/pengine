#include "commands.h"
#include <engine/macros.h>
#include <engine/renderer/vulkan/vulkan.h>
#include <engine/renderer/vulkan/framebuffer.h>
#include <vulkan/vulkan_core.h>
#include "render_pass.h"



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

void pe_vk_record_commands_buffer(int i){

    VkCommandBufferBeginInfo begininfo;
    ZERO(begininfo);
    begininfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begininfo.flags = 0;
    begininfo.pInheritanceInfo = NULL;

    VkCommandBuffer *buffer = array_get(&pe_vk_command_buffers, i);
    vkResetCommandBuffer(*(buffer),0);

    vkBeginCommandBuffer(*(buffer), &begininfo);
    pe_vk_start_render_pass(i);
}
void pe_vk_command_init() {


  array_init(&pe_vk_command_buffers, sizeof(VkCommandBuffer),
             pe_vk_framebuffers.count);
  array_resize(&pe_vk_command_buffers, pe_vk_framebuffers.count);

  VkCommandBufferAllocateInfo bufferinfo;
  ZERO(bufferinfo);
  bufferinfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  bufferinfo.commandPool = pe_vk_commands_pool;
  bufferinfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  bufferinfo.commandBufferCount = pe_vk_command_buffers.count;

  vkAllocateCommandBuffers(vk_device, &bufferinfo, pe_vk_command_buffers.data);

}


void pe_vk_commands_end(int i) {
  VkCommandBuffer *buffer = array_get(&pe_vk_command_buffers, i);
  vkEndCommandBuffer(*(buffer));
}
