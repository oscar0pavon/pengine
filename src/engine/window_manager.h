#ifndef PE_WINDOWS_MANAGER_H
#define PE_WINDOWS_MANAGER_H



#define INIT_WINDOW_SIZE_X 1280
#define INIT_WINDOW_SIZE_Y 720

#include <engine/camera.h>

typedef enum PERendererType { PEWMVULKAN, PEWMOPENGLES2 } PERendererType;

typedef struct EngineWindow {
  Array tabs;
  char name[20];
  bool focus;
  bool initialized;
  void (*draw)(void);
  void (*init)(void);
  void (*finish)(void);
  void (*input)(void);
  void (*char_parser)(unsigned char);
  CameraComponent camera;
} EngineWindow;

void pe_wm_init();
void window_update_viewport(int width, int height);
bool is_wm_swapped();

void pe_wm_configure_window(EngineWindow*);


void pe_wm_create_window(EngineWindow *, EngineWindow *share_window,
                   const char *name);

void pe_wm_window_init(EngineWindow *window);

void pe_wm_create_x11_window();

void window_initialize_windows();

void window_manager_create_editor_windows_data();

void window_set_focus(EngineWindow *window);

void windows_manager_init();

void window_update_windows_input();

void window_manager_update_windows_input();

bool pe_wm_should_close(EngineWindow *);

#ifdef ANDROID
void pe_wm_egl_init();

void pe_wm_egl_end();
#endif

void pe_wm_swap_buffers();

void pe_wm_make_context(EngineWindow*window);

void pe_wm_events_update();

void pe_wm_input_update();

void pe_wm_windows_draw();

float actual_window_width;
float actual_window_height;

bool editor_window_content_open;

EngineWindow *window_editor_main;

EngineWindow *current_window;

Array engine_windows;

PERendererType pe_renderer_type;

bool pe_is_window_init;

bool pe_is_window_terminate;

bool pe_is_glfw_window_created;

#endif // !ENGINE_WINDOWS_MANAGER_H
