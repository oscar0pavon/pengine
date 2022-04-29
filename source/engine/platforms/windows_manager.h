#ifndef PE_WINDOWS_MANAGER_H
#define PE_WINDOWS_MANAGER_H

#define GLFW_INCLUDE_ES2
#define GLFW_INCLUDE_GLEXT

#ifdef LINUX

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#endif

#include <engine/camera.h>
#ifdef EDITOR
#include "../../../editor/windows/static_mesh_editor.h"
#include "../../../editor/windows/tabs.h"
#endif // EDITOR

#define INIT_WINDOW_SIZE_X 1280
#define INIT_WINDOW_SIZE_Y 720

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
#ifdef EDITOR
  u8 tab_current_id;
  EditorTab *tab_current;
#endif
#ifdef LINUX
  GLFWwindow *window;
#endif
} EngineWindow;

#ifdef LINUX
void window_resize_callback(GLFWwindow *, int width, int height);
void window_focus_callback(GLFWwindow *, int);
#endif

void window_create(EngineWindow *, EngineWindow *share_window,
                   const char *name);

void pe_wm_window_init(EngineWindow *window);
void window_manager_draw_windows();
void window_initialize_windows();

void window_update_viewport();

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

#endif // !ENGINE_WINDOWS_MANAGER_H
