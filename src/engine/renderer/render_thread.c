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

  pe_vk_draw_frame();
}
