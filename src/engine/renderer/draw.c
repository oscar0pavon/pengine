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

void pe_vk_draw_commands(VkCommandBuffer *cmd_buffer, uint32_t index) {

  vkCmdSetViewport(*cmd_buffer, 0, 1, &viewport);

  vkCmdSetScissor(*cmd_buffer, 0, 1, &scissor);

  VkDeviceSize offsets[] = {0};

  // TODO draw objets here

  if (pe_vk_draw_scene)
    pe_vk_draw_scene(cmd_buffer, index);
}

void pe_vk_draw_frame() {

  vkWaitForFences(vk_device, 1, &pe_vk_fence_in_flight, VK_TRUE, UINT64_MAX);
  vkResetFences(vk_device, 1, &pe_vk_fence_in_flight);

  uint32_t image_index;

  vkAcquireNextImageKHR(vk_device, pe_vk_swap_chain, UINT64_MAX,
                        pe_vk_semaphore_images_available, VK_NULL_HANDLE,
                        &image_index);

  VkCommandBuffer current_command = pe_vk_start_record_command(image_index);

  VkImage current_swapchain_image = pe_vk_swch_images[image_index];

  pe_vk_image_to_color_attacthment(current_swapchain_image);

  pe_vk_start_render_pass(current_command, image_index);//INFO this is where we draw things


  pe_vk_end_command(current_command);

  VkSemaphore singal_semaphore[] = {pe_vk_semaphore_render_finished};
  VkSemaphore wait_semaphores[] = {pe_vk_semaphore_images_available};
  

  VkPipelineStageFlags wait_stages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  VkSwapchainKHR swap_chains[] = {pe_vk_swap_chain};

  VkSubmitInfo submit_info = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                              .waitSemaphoreCount = 1,
                              .pWaitSemaphores = wait_semaphores,
                              .pWaitDstStageMask = wait_stages,
                              .commandBufferCount = 1,
                              .pCommandBuffers = &current_command,
                              .signalSemaphoreCount = 1,
                              .pSignalSemaphores = singal_semaphore};

  vkQueueSubmit(vk_queue, 1, &submit_info, pe_vk_fence_in_flight);

  if(is_drm_rendering){
    vkWaitForFences(vk_device,1, &pe_vk_fence_in_flight, VK_TRUE, UINT64_MAX);
  }

  VkPresentInfoKHR present_info = {

      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = singal_semaphore,
      .swapchainCount = 1,
      .pSwapchains = swap_chains,
      .pImageIndices = &image_index};

  VKVALID(vkQueuePresentKHR(vk_queue, &present_info), "Can't present");

  vkQueueWaitIdle(vk_queue);
}
