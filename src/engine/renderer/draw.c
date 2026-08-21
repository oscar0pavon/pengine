#include "draw.h"
#include "commands.h"
#include "descriptor_set.h"
#include "pipeline.h"
#include "swap_chain.h"

#include "sync.h"
#include "uniform_buffer.h"
#include "vk_vertex.h"
#include "vulkan.h"
#include <engine/macros.h>
#include <stdint.h>
#include <sys/types.h>
#include <vulkan/vulkan_core.h>
#include "render_pass.h"
#include "vk_images.h"

#include "engine/renderer/renderer.h"

void (*pe_vk_draw_scene)(VkCommandBuffer *cmd_buffer, uint32_t index) = NULL;

#include "descriptor_set.h"

void pe_vk_draw_model(PDrawModelCommand *draw_model) {

  VkDeviceSize offsets[] = {0};

  VkDescriptorSet *descriptor_set = NULL;

  descriptor_set =
      array_get(&draw_model->model->descriptor_sets, draw_model->image_index);

  VkCommandBuffer command = draw_model->command_buffer;
  vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          draw_model->layout, 0, 1, descriptor_set, 0, NULL);
  vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    draw_model->model->shader.pipeline);

  vkCmdBindVertexBuffers(command, 0, 1, &draw_model->model->vertex_buffer.buffer, offsets);
  vkCmdBindIndexBuffer(command, draw_model->model->index_buffer.buffer, 0,
                       VK_INDEX_TYPE_UINT16);
  vkCmdDrawIndexed(command, draw_model->model->index_array.count, 1, 0, 0, 0);
}

//draws instance_count copies of the mesh in one call, each one placed by
//an entry in instance_buffer. needs a pipeline built with
//pe_vk_create_shader_instanced()
void pe_vk_draw_model_instanced(PDrawModelCommand *draw_model,
                                VkBuffer instance_buffer,
                                uint32_t instance_count) {

  VkDeviceSize offsets[] = {0};

  VkDescriptorSet *descriptor_set =
      array_get(&draw_model->model->descriptor_sets, draw_model->image_index);

  VkCommandBuffer command = draw_model->command_buffer;
  vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          draw_model->layout, 0, 1, descriptor_set, 0, NULL);
  vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    draw_model->model->shader.pipeline);

  vkCmdBindVertexBuffers(command, 0, 1,
                         &draw_model->model->vertex_buffer.buffer, offsets);
  vkCmdBindVertexBuffers(command, 1, 1, &instance_buffer, offsets);

  vkCmdBindIndexBuffer(command, draw_model->model->index_buffer.buffer, 0,
                       VK_INDEX_TYPE_UINT16);

  vkCmdDrawIndexed(command, draw_model->model->index_array.count,
                   instance_count, 0, 0, 0);
}

void pe_vk_draw_commands(PRenderTarget *target, VkCommandBuffer *cmd_buffer,
                         uint32_t index) {

  vkCmdSetViewport(*cmd_buffer, 0, 1, &target->viewport);

  vkCmdSetScissor(*cmd_buffer, 0, 1, &target->scissor);

  VkDeviceSize offsets[] = {0};

  // TODO draw objets here

  if (pe_vk_draw_scene)
    pe_vk_draw_scene(cmd_buffer, index);
}

void pe_vk_draw_frame(PRenderTarget *target) {

  VkFence *frame_fence = &target->fence_in_flight[target->current_frame];

  //the frame two submissions back, whose semaphores and per frame state this
  //one is about to reuse
  vkWaitForFences(vk_device, 1, frame_fence, VK_TRUE, UINT64_MAX);

  uint32_t image_index;

  vkAcquireNextImageKHR(vk_device, target->swap_chain, UINT64_MAX,
                        target->semaphore_images_available[target->current_frame],
                        VK_NULL_HANDLE, &image_index);

  //INFO waiting on this frame's own fence says nothing about the image the
  //acquire handed back. everything the recording below overwrites is indexed by
  //image_index - the command buffer, and every model's uniform buffer and
  //descriptor set - so the frame that last rendered into this image has to be
  //done before any of it is touched
  if (target->fence_image_in_flight[image_index] != VK_NULL_HANDLE)
    vkWaitForFences(vk_device, 1, &target->fence_image_in_flight[image_index],
                    VK_TRUE, UINT64_MAX);

  target->fence_image_in_flight[image_index] = *frame_fence;

  //reset last: a fence that is waited on above must still be signalled there
  vkResetFences(vk_device, 1, frame_fence);

  VkCommandBuffer current_command =
      pe_vk_start_record_command(target, image_index);

  pe_vk_start_render_pass(target, current_command, image_index);//INFO this is where we draw things


  pe_vk_end_command(current_command);

  VkSemaphore singal_semaphore[] = {
      target->semaphore_render_finished[image_index]};
  VkSemaphore wait_semaphores[] = {
      target->semaphore_images_available[target->current_frame]};


  VkPipelineStageFlags wait_stages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  VkSwapchainKHR swap_chains[] = {target->swap_chain};

  VkSubmitInfo submit_info = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                              .waitSemaphoreCount = 1,
                              .pWaitSemaphores = wait_semaphores,
                              .pWaitDstStageMask = wait_stages,
                              .commandBufferCount = 1,
                              .pCommandBuffers = &current_command,
                              .signalSemaphoreCount = 1,
                              .pSignalSemaphores = singal_semaphore};

  vkQueueSubmit(vk_queue, 1, &submit_info, *frame_fence);

  if(is_drm_rendering){
    vkWaitForFences(vk_device, 1, frame_fence, VK_TRUE, UINT64_MAX);
  }

  VkPresentInfoKHR present_info = {

      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = singal_semaphore,
      .swapchainCount = 1,
      .pSwapchains = swap_chains,
      .pImageIndices = &image_index};

  VKVALID(vkQueuePresentKHR(vk_queue, &present_info), "Can't present");

  //INFO no vkQueueWaitIdle here. it made every frame end with the queue empty,
  //which is the whole of what the fences and semaphores above are for; with it
  //in place PE_VK_FRAMES_IN_FLIGHT could be any number and nothing would ever
  //overlap
  target->current_frame = (target->current_frame + 1) % PE_VK_FRAMES_IN_FLIGHT;
}
