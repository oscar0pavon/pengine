#include "render_thread.h"
#include <pengine/engine.h>
#include <pengine/renderer/draw.h>
#include <pengine/renderer/renderer.h>
#include <pengine/threads.h>
#include <pengine/types.h>
#include <pengine/window_manager.h>


//INFO the engine has no scene of its own to walk. the render pass calls the
//application back through pe_vk_draw_scene, and that hook is where the models
//to draw are chosen and recorded
void pe_frame_draw() {

  pe_wm_apply_resize();

  //INFO sequential on purpose: in DRM mode pe_vk_draw_frame() blocks on the
  //frame's fence right after submit (draw.c), so target k's GPU work is
  //provably finished before target k+1's CPU side starts rewriting the
  //per-model uniform buffers those two targets share. more than one target
  //therefore only ever exists on the DRM path (see pe_vk_init())
  for (u32 i = 0; i < pe_render_targets_count; i++)
    pe_vk_draw_frame(&pe_render_targets[i]);
}
