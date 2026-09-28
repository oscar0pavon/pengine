#include "render_thread.h"
#include <engine/engine.h>
#include <engine/renderer/draw.h>
#include <engine/renderer/renderer.h>
#include <engine/threads.h>
#include <engine/types.h>

void pe_frame_clean() {}

void pe_render_thread_init() {

  pe_vk_init();
}


//INFO the engine has no scene of its own to walk. the render pass calls the
//application back through pe_vk_draw_scene, and that hook is where the models
//to draw are chosen and recorded
void pe_frame_draw() {

  //INFO sequential on purpose: in DRM mode pe_vk_draw_frame() blocks on the
  //frame's fence right after submit (draw.c), so target k's GPU work is
  //provably finished before target k+1's CPU side starts rewriting the
  //per-model uniform buffers those two targets share. more than one target
  //therefore only ever exists on the DRM path (see pe_vk_init())
  for (u32 i = 0; i < pe_render_targets_count; i++)
    pe_vk_draw_frame(&pe_render_targets[i]);
}
