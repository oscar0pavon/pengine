#ifndef PE_WINDOWS_MANAGER_H
#define PE_WINDOWS_MANAGER_H



#define INIT_WINDOW_SIZE_X 1280
#define INIT_WINDOW_SIZE_Y 720

#include <engine/camera.h>

void pe_wm_init();
void window_update_viewport(int width, int height);
bool is_wm_swapped();

void pe_create_window();

void window_initialize_windows();

void window_manager_create_editor_windows_data();


void windows_manager_init();

void window_update_windows_input();

void window_manager_update_windows_input();

void pe_wm_events_update();

//takes the size the compositor last configured and rebuilds what is sized
//from it. call it between two frames, from the thread that draws
void pe_wm_apply_resize(void);

//while locked the cursor stays put and mouse.dx/dy collect the motion
void pe_wm_lock_pointer(void);
void pe_wm_unlock_pointer(void);

void pe_wm_input_update();

float actual_window_width;
float actual_window_height;

bool editor_window_content_open;

bool pe_is_window_init;

bool pe_is_window_terminate;

#endif // !PE_WINDOWS_MANAGER_H
