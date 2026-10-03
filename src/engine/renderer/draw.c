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
#include "engine/text.h"

void (*pe_vk_draw_scene)(PRenderTarget *target, VkCommandBuffer *cmd_buffer,
                         uint32_t index) = NULL;

#include "descriptor_set.h"

//the accessor a model's indices were loaded from decides how wide they are
//(pe_loader_mesh_read_accessor_indices() sizes index_array to match) - a
//glTF exporter that can produce more than 65535 vertices, m22gltf among
//them, writes them as 32 bit, and binding that here as UINT16 would read
//every pair of indices as one, wrecking the mesh's topology
static VkIndexType pe_vk_model_index_type(PModel *model) {
  return model->index_array.element_bytes_size == 4 ? VK_INDEX_TYPE_UINT32
                                                     : VK_INDEX_TYPE_UINT16;
}

//a model of parts: one draw for each, with the descriptor set that holds the
//part's textures and its factors pushed. the layout has to be
//pe_vk_pipeline_layout_pbr, and the environment set the caller names is bound
//as its second set
static void pe_vk_draw_model_parts(PDrawModelCommand *draw_model) {
  PModel *model = draw_model->model;
  VkCommandBuffer command = draw_model->command_buffer;
  VkDeviceSize offsets[] = {0};
  u32 images = pe_vk_targets_max_images_count();

  vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    model->shader.pipeline);
  vkCmdBindVertexBuffers(command, 0, 1, &model->vertex_buffer.buffer, offsets);
  vkCmdBindIndexBuffer(command, model->index_buffer.buffer, 0,
                       pe_vk_model_index_type(model));
  vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          draw_model->layout, 1, 1, &draw_model->environment, 0,
                          NULL);

  for (u32 i = 0; i < model->parts.count; i++) {
    PModelPart *part = array_get(&model->parts, i);
    VkDescriptorSet *set = array_get(&model->part_descriptor_sets,
                                     i * images + draw_model->image_index);
    vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            draw_model->layout, 0, 1, set, 0, NULL);
    vkCmdPushConstants(command, draw_model->layout, VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(PPartMaterial), &part->material);
    vkCmdDrawIndexed(command, part->index_count, 1, part->first_index, 0, 0);
  }
}

void pe_vk_draw_model(PDrawModelCommand *draw_model) {

  if (draw_model->model->parts.count > 0) {
    pe_vk_draw_model_parts(draw_model);
    return;
  }

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
                       pe_vk_model_index_type(draw_model->model));

  PModel *model = draw_model->model;
  u32 main_count = model->index_array.count;
  if (model->has_extra_texture && model->extra_first_index < main_count)
    main_count = model->extra_first_index;
  vkCmdDrawIndexed(command, main_count, 1, 0, 0, 0);

  if (main_count == model->index_array.count)
    return;

  descriptor_set = array_get(&model->extra_descriptor_sets,
                             draw_model->image_index);
  vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          draw_model->layout, 0, 1, descriptor_set, 0, NULL);
  vkCmdDrawIndexed(command, model->index_array.count - main_count, 1,
                   main_count, 0, 0);
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
                       pe_vk_model_index_type(draw_model->model));

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
    pe_vk_draw_scene(target, cmd_buffer, index);
}

void pe_vk_draw_frame(PRenderTarget *target) {

  VkFence *frame_fence = &target->fence_in_flight[target->current_frame];

  //the frame two submissions back, whose semaphores and per frame state this
  //one is about to reuse
  vkWaitForFences(vk_device, 1, frame_fence, VK_TRUE, UINT64_MAX);

  uint32_t image_index;

  VkResult acquired = vkAcquireNextImageKHR(
      vk_device, target->swap_chain, UINT64_MAX,
      target->semaphore_images_available[target->current_frame],
      VK_NULL_HANDLE, &image_index);

  //INFO the swap chain no longer matches the surface: the window was resized
  //under us, or the compositor changed something about it. nothing was
  //acquired and the semaphore was not signalled, so this frame is dropped
  //either way, and the next one draws against the rebuilt chain. an
  //application that rebuilds on its own configure events rarely gets here -
  //this is the case where the surface changed without asking.
  //
  //only on the window path: a DRM display surface has no compositor to resize
  //it, its extent comes from the mode, and rebuilding a scanout chain per
  //frame is not something to start doing behind a compositor's back
  if (acquired == VK_ERROR_OUT_OF_DATE_KHR) {

    if (!is_drm_rendering)
      pe_vk_recreate_swapchain(target);
    else
      LOG("Display swap chain is out of date\n");

    return;
  }

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

  //INFO pfonts_vulkan_sync_atlas() (called through here) records and submits
  //its own one-shot transfer command buffer to upload the glyph atlas -
  //Vulkan does not allow that inside an active render pass instance, so this
  //has to run before pe_vk_start_render_pass() opens one, not after
  pe_text_sync();

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

  VkResult presented = vkQueuePresentKHR(vk_queue, &present_info);

  //SUBOPTIMAL presents fine but says the chain no longer fits the surface, so
  //it is rebuilt for the next frame rather than left to degrade. same reason
  //as the acquire above for leaving the DRM path alone
  if ((presented == VK_ERROR_OUT_OF_DATE_KHR ||
       presented == VK_SUBOPTIMAL_KHR) &&
      !is_drm_rendering)
    pe_vk_recreate_swapchain(target);
  else if (presented != VK_SUCCESS && presented != VK_SUBOPTIMAL_KHR)
    LOG("Can't present \n");

  //INFO no vkQueueWaitIdle here. it made every frame end with the queue empty,
  //which is the whole of what the fences and semaphores above are for; with it
  //in place PE_VK_FRAMES_IN_FLIGHT could be any number and nothing would ever
  //overlap
  target->current_frame = (target->current_frame + 1) % PE_VK_FRAMES_IN_FLIGHT;
}
