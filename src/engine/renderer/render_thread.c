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

//INFO thread_new_detached() hands this straight to pthread_create(), so it has
//to be the entry point type. it was a void(void) passed through a parameter
//declared void*(*)(void*)
void *pe_render_draw(void *argument) {

  pe_thread_control(&render_thread_commads);

  if (render_thread_definition.draw != NULL)
    render_thread_definition.draw();

  return NULL;
}

/*Start render thread and call pe_render_draw()*/
void pe_render_thread_start_and_draw() {
  thread_new_detached(pe_render_draw, NULL, "Render", &pe_th_render_id);
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
