#ifndef PE_WINDOWS_MANAGER_H
#define PE_WINDOWS_MANAGER_H



#define INIT_WINDOW_SIZE_X 1280
#define INIT_WINDOW_SIZE_Y 720

#include <engine/camera.h>

#include <X11/Xlib.h>

typedef enum PERendererType { PEWMVULKAN, PEWMOPENGLES2 } PERendererType;


void pe_wm_init();
void window_update_viewport(int width, int height);
bool is_wm_swapped();


void pe_wm_create_x11_window();

void window_initialize_windows();

void window_manager_create_editor_windows_data();


void windows_manager_init();

void window_update_windows_input();

void window_manager_update_windows_input();

#ifdef ANDROID
void pe_wm_egl_init();

void pe_wm_egl_end();
#endif

void pe_wm_swap_buffers();

void pe_wm_events_update();

void pe_wm_input_update();

float actual_window_width;
float actual_window_height;

bool editor_window_content_open;

PERendererType pe_renderer_type;

bool pe_is_window_init;

bool pe_is_window_terminate;

Display                 *display;
Window                  window;

#endif // !ENGINE_WINDOWS_MANAGER_H
