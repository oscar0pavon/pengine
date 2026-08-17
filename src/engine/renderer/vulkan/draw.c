#include "draw.h"
#include "commands.h"
#include "descriptor_set.h"
#include "pipeline.h"
#include "swap_chain.h"
#include "sync.h"
#include "uniform_buffer.h"
#include "vk_vertex.h"
#include "vulkan.h"
#include <engine/engine.h>
#include <engine/macros.h>
#include <vulkan/vulkan_core.h>

void pe_vk_draw_model(int i, PModel *model) {

  VkCommandBuffer *cmd_buffer = array_get(&pe_vk_command_buffers, i);

  vkCmdBindPipeline(*(cmd_buffer), VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pe_vk_pipeline);

  VkBuffer vertex_buffers[] = {model->vertex_buffer};
  VkDeviceSize offsets[] = {0};

  vkCmdBindVertexBuffers(*(cmd_buffer), 0, 1, vertex_buffers, offsets);

  vkCmdBindIndexBuffer(*(cmd_buffer), model->index_buffer, 0,
                       VK_INDEX_TYPE_UINT16);

  // vkCmdDraw(*(cmd_buffer), vertices.count, 1, 0, 0);

  //    VkDescriptorSet* set = array_get(&pe_vk_descriptor_sets,i);

  //   vkCmdBindDescriptorSets(*(cmd_buffer),VK_PIPELINE_BIND_POINT_GRAPHICS,pe_vk_pipeline_layout,0,1,set,0,NULL);

  vkCmdDrawIndexed(*(cmd_buffer), model->index_array.count, 1, 0, 0, 0);
}

void pe_vk_draw_simple_model(int i) {

  pe_vk_draw_model(i, test_model);
  // LOG("drawing model");
  // pe_vk_draw_model(i,test_model2);
}
void pe_vk_draw_commands(VkCommandBuffer *cmd_buffer, uint32_t index) {

  VkOffset2D offset = {0, 0};

  VkViewport viewport;
  ZERO(viewport);
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)pe_vk_swch_extent.width;
  viewport.height = (float)pe_vk_swch_extent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(*(cmd_buffer), 0, 1, &viewport);

  VkRect2D scissor;
  scissor.extent = pe_vk_swch_extent;
  scissor.offset = offset;

  vkCmdSetScissor(*(cmd_buffer), 0, 1, &scissor);

  VkDeviceSize offsets[] = {0};

  VkDescriptorSet *set = NULL;

  // VkPipeline* triangle_pipeline = array_get(&pe_graphics_pipelines, 1);
  //
  // vkCmdBindPipeline(*(cmd_buffer),VK_PIPELINE_BIND_POINT_GRAPHICS,*(triangle_pipeline));
  //
  // vkCmdDraw(*(cmd_buffer), 3,1,0,0);
  //
  //
  //
  // vkCmdBindPipeline(*(cmd_buffer),VK_PIPELINE_BIND_POINT_GRAPHICS,pe_vk_pipeline);
  //
  // vkCmdDraw(*(cmd_buffer), 3,1,0,0);
  //
  //
  //
  //
  // VkPipeline* in_position = array_get(&pe_graphics_pipelines, 2);
  //
  // vkCmdBindPipeline(*(cmd_buffer),VK_PIPELINE_BIND_POINT_GRAPHICS,*(in_position));
  //
  // vkCmdBindVertexBuffers(*(cmd_buffer), 0, 1, &test_model->vertex_buffer ,
  // offsets); vkCmdDraw(*(cmd_buffer), test_model->vertex_array.count , 1, 0,
  // 0);

  // ############################################################
  // ############### with descriptor set ########################
  // pe_vk_uniform_buffer_update_one(index);
  VkPipeline *uniform = array_get(&pe_graphics_pipelines, 3);
  //
  // VkDescriptorSet *set = array_get(&test_model->descriptor_sets, index);
  //
  // vkCmdBindDescriptorSets(*(cmd_buffer), VK_PIPELINE_BIND_POINT_GRAPHICS,
  //                         pe_vk_pipeline_layout_with_descriptors, 0, 1, set,
  //                         0, NULL);
  // vkCmdBindPipeline(*(cmd_buffer), VK_PIPELINE_BIND_POINT_GRAPHICS,
  // *(uniform));
  //
  // vkCmdBindVertexBuffers(*(cmd_buffer), 0, 1, &test_model->vertex_buffer,
  //                        offsets);
  // vkCmdBindIndexBuffer(*(cmd_buffer), test_model->index_buffer, 0,
  //                      VK_INDEX_TYPE_UINT16);
  // vkCmdDrawIndexed(*(cmd_buffer), test_model->index_array.count, 1, 0, 0, 0);

  // ############################################################
  // ############### with descriptor set ########################

  //
  pe_vk_uniform_buffer_update_two(index);
  set = array_get(&test_model2->descriptor_sets, index);

  vkCmdBindDescriptorSets(*(cmd_buffer), VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pe_vk_pipeline_layout_with_descriptors, 0, 1, set, 0,
                          NULL);
  vkCmdBindPipeline(*(cmd_buffer), VK_PIPELINE_BIND_POINT_GRAPHICS, *(uniform));

  vkCmdBindVertexBuffers(*(cmd_buffer), 0, 1, &test_model2->vertex_buffer,
                         offsets);
  vkCmdBindIndexBuffer(*(cmd_buffer), test_model2->index_buffer, 0,
                       VK_INDEX_TYPE_UINT16);
  vkCmdDrawIndexed(*(cmd_buffer), test_model2->index_array.count, 1, 0, 0, 0);
  // ############################################################
  // ############### Skeletal ########################

  // VkPipeline *skeletal_pipeline = array_get(&pe_graphics_pipelines, 4);
  // pe_vk_uniform_buffer_update_skeletal(index);
  // set = array_get(&anim_model->descriptor_sets, index);
  //
  // vkCmdBindDescriptorSets(*(cmd_buffer), VK_PIPELINE_BIND_POINT_GRAPHICS,
  //                         pe_vk_pipeline_layout_skinned, 0, 1, set, 0, NULL);
  // vkCmdBindPipeline(*(cmd_buffer), VK_PIPELINE_BIND_POINT_GRAPHICS,
  //                   *(skeletal_pipeline));
  //
  // vkCmdBindVertexBuffers(*(cmd_buffer), 0, 1, &anim_model->vertex_buffer,
  //                        offsets);
  // vkCmdBindIndexBuffer(*(cmd_buffer), anim_model->index_buffer, 0,
  //                      VK_INDEX_TYPE_UINT16);
  // vkCmdDrawIndexed(*(cmd_buffer), anim_model->index_array.count, 1, 0, 0, 0);

  //############################################################################3
  // vkCmdDraw(*(cmd_buffer), test_model2->vertex_array.count, 1, 0, 0);
  //  pe_vk_draw_model(i,test_model);
}
void pe_vk_draw_frame() {

  vkWaitForFences(vk_device, 1, &pe_vk_fence_in_flight, VK_TRUE, UINT64_MAX);
  vkResetFences(vk_device, 1, &pe_vk_fence_in_flight);

  uint32_t image_index;

  vkAcquireNextImageKHR(vk_device, pe_vk_swap_chain, UINT64_MAX,
                        pe_vk_semaphore_images_available, VK_NULL_HANDLE,
                        &image_index);

  // pe_vk_uniform_buffer_update(image_index);
  pe_vk_record_commands_buffer(image_index);

  VkSemaphore singal_semaphore[] = {pe_vk_semaphore_render_finished};
  VkSemaphore wait_semaphores[] = {pe_vk_semaphore_images_available};
  VkCommandBuffer *cmd_buffer = array_get(&pe_vk_command_buffers, image_index);

  VkPipelineStageFlags wait_stages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  VkSwapchainKHR swap_chains[] = {pe_vk_swap_chain};

  VkSubmitInfo submit_info;
  ZERO(submit_info);
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = wait_semaphores;
  submit_info.pWaitDstStageMask = wait_stages;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = cmd_buffer;
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores = singal_semaphore;

  vkQueueSubmit(vk_queue, 1, &submit_info, pe_vk_fence_in_flight);

  VkPresentInfoKHR present_info;
  ZERO(present_info);
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = singal_semaphore;
  present_info.swapchainCount = 1;
  present_info.pSwapchains = swap_chains;
  present_info.pImageIndices = &image_index;

  VKVALID(vkQueuePresentKHR(vk_queue, &present_info), "Can't present");

  vkQueueWaitIdle(vk_queue);
}
