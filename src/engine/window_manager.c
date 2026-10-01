#include "window_manager.h"
#include "engine/camera.h"
#include <engine/log.h>

#include <stdio.h>

#include <engine/engine.h>
#include <engine/game.h>
#include <engine/input.h>

#include <pway/pway.h>

#include <engine/renderer/renderer.h>

bool pe_wm_swapped = false;

//pway reports the raw evdev code and a WL_KEYBOARD_KEY_STATE_*, and
//PE_KEY_PRESSED/RELEASED are those two values, so the state goes straight on
static void pe_wm_key_event(uint32_t key_code, uint32_t state) {
  pe_parse_key_event(key_code, state);
}

static void pe_wm_mouse_motion(void) {
  pe_parse_mouse_motion(pway->mouse.x, pway->mouse.y);
}

//pway calls click()/click_release() with no argument and expects the app to
//read which button off pway->mouse.current_button - it is only ever NULL for
//a button pway does not track (there is none, currently), never for the
//wheel, which pway resolves through click_release() alone
static void pe_wm_mouse_click(void) {
  if (pway->mouse.current_button)
    pe_parse_mouse_button(pway->mouse.current_button->id, true);
}

static void pe_wm_mouse_click_release(void) {
  if (pway->mouse.current_button)
    pe_parse_mouse_button(pway->mouse.current_button->id, false);
}

//fires from pway_handle_events() once the compositor has already torn the
//toplevel down (the close button, or the connection being lost), so this
//only has to stop the main loop - pe_end() does the real teardown afterwards
static void pe_wm_exit_event(void) {
  pe_terminate();
}

void pe_wm_input_update() {
}

//INFO the viewport is the swap chain extent, which pe_vk_create_swap_chain
//reads from pe_window_width/pe_window_height. nothing outside vulkan sets a
//viewport any more, so this only has to keep the camera's numbers in step
void window_update_viewport(int width, int height){
  camera_width_screen = width;
  camera_height_screen = height;
}

bool is_wm_swapped(){
	if(pe_wm_swapped){
		pe_wm_swapped=false;
	}
	return true;
}



void pe_wm_events_update() {
  pway_handle_events();
}

void pe_create_window(){

  pway = pway_init();

  pway->key = &pe_wm_key_event;
  pway->exit = &pe_wm_exit_event;
  pway->update_mouse = &pe_wm_mouse_motion;
  pway->click = &pe_wm_mouse_click;
  pway->click_release = &pe_wm_mouse_click_release;

  //INFO no pway_init_egl() here. it binds a desktop GL context and hangs a
  //wl_egl_window off pway_surface - the same wl_surface pe_vk_create_surface
  //hands to VkWaylandSurfaceKHR, so both would be driving one surface
  pway_create_window(game->name, INIT_WINDOW_SIZE_X, INIT_WINDOW_SIZE_Y);
  pway_set_app_id(game->name);

  //INFO the camera aspect, the 2D projection and the viewport all read these,
  //so they come from the window rather than from a constant repeated here. pway
  //reports the size that was asked for and does not yet follow the compositor's
  //configure, so a compositor that hands out a different size still gets a
  //frame drawn for the requested one
  pe_window_width = pway->width;
  pe_window_height = pway->height;

  actual_window_width = pway->width;
  actual_window_height = pway->height;

  window_update_viewport(pway->width, pway->height);
}
