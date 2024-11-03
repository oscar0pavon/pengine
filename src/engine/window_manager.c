#include "window_manager.h"
#include "platforms/windows_manager.h"
#include "windows_manager.h"
#include <engine/log.h>

#ifdef ANDROID
#include <engine/platforms/android/android.h>
#include <EGL/egl.h>
EGLDisplay display;
EGLSurface surface;
EGLContext context;
#include <GLES/gl.h>
#endif

#include <engine/game.h>
#include <engine/input.h>
#include <engine/platforms/windows_manager.h>
#include <engine/window_manager.h>

bool pe_wm_swapped = false;

#ifdef ANDROID
void pe_wm_egl_init(){
	// Setup OpenGL ES 2
	// http://stackoverflow.com/questions/11478957/how-do-i-create-an-opengl-es-2-context-in-a-native-activity

	const EGLint attribs[] = {
			EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, //important
			EGL_BLUE_SIZE, 8,
			EGL_GREEN_SIZE, 8,
			EGL_RED_SIZE, 8,
			EGL_DEPTH_SIZE,8,
			EGL_NONE
	};

	EGLint attribList[] =
	{
			EGL_CONTEXT_CLIENT_VERSION, 2,
			EGL_NONE
	};

	EGLint w, h, dummy, format;
	EGLint numConfigs;
	EGLConfig config;


	display = eglGetDisplay(EGL_DEFAULT_DISPLAY);


	eglInitialize(display, 0, 0);

	/* Here, the application chooses the configuration it desires. In this
	 * sample, we have a very simplified selection process, where we pick
	 * the first EGLConfig that matches our criteria */
	eglChooseConfig(display, attribs, &config, 1, &numConfigs);

	/* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
	 * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
	 * As soon as we picked a EGLConfig, we can safely reconfigure the
	 * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
	eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);

	if(game == NULL){

		LOG("game null");
	}

	if(game->app == NULL){
		LOG("game->app null");
	}

	if(game->app->window == NULL){
		LOG("game->app->window is null");
	}
	
	ANativeWindow_setBuffersGeometry(game->app->window, 0, 0, format);
	surface = eglCreateWindowSurface(display, config, game->app->window, NULL);

	context = eglCreateContext(display, config, NULL, attribList);

	if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
		LOG("Unable to eglMakeCurrent");
		return -1;
	}

	// Grab the width and height of the surface
	eglQuerySurface(display, surface, EGL_WIDTH, &w);
	eglQuerySurface(display, surface, EGL_HEIGHT, &h);
	
	// Initialize GL state.
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
	glEnable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

  camera_heigth_screen = h;
  camera_width_screen = w;
  window_update_viewport(w, h);
	LOG("EGL initialized");
	return 0;

}
void pe_wm_egl_context_make_current(){
	eglMakeCurrent(display, surface, surface, context);
}
/**
 * Tear down the EGL context currently associated with the display.
 */
void pe_wm_egl_end() {
	if (display != EGL_NO_DISPLAY) {
		eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		if (context != EGL_NO_CONTEXT) {
			eglDestroyContext(display, context);
		}
		if (surface != EGL_NO_SURFACE) {
			eglDestroySurface(display, surface);
		}
		eglTerminate(display);
	}
	display = EGL_NO_DISPLAY;
	context = EGL_NO_CONTEXT;
	surface = EGL_NO_SURFACE;
}
#endif


void pe_wm_input_update() {

  // Draw tab bar 	& draw current tabb
  for (u8 i = 0; i < engine_windows.count; i++) {
    EngineWindow *window = array_get(&engine_windows, i);
		if (!window->initialized){
      continue;
		}

#ifdef LINUX
    // The mouse need to stay in the window for window->input call
    if (window->focus) {
      if (window->input)
        window->input();
    }
#endif

#ifdef ANDROID

		if (window->input){

      window->input();
		}
#endif
  }
}

bool pe_wm_should_close(EngineWindow* window){
	if(pe_is_window_terminate)	
		return true;
	else
    return false;
}



void pe_wm_init(){

#ifdef DESKTOP
windows_manager_init();
#endif
}


void window_update_viewport(int width, int height){
		glViewport(0,0,width,height);
#ifdef PE_FREETYPE
    text_renderer_update_pixel_size();
#endif
		camera_update_aspect_ratio(&current_window->camera);
}

void pe_wm_configure_window(EngineWindow* win){
  
	if (win == NULL) { 
		LOG("ERROR: Window not found\n");
    return;
  }
	if (win->initialized){
		LOG("Window already initialized\n");
    return;
	}

  current_window = win;


#ifdef LINUX
	pe_wm_create_window(win,NULL, "PavonEngine");
	glfwSetKeyCallback(win->window, pe_input_key_callback);
	glfwSetCursorPosCallback(win, pe_input_mouse_movement_callback);
	glfwSetMouseButtonCallback(win, pe_input_mouse_button_callback);
	glfwSetCharCallback(win->window, pe_input_character_callback);

	glfwSetWindowFocusCallback(win->window,window_focus_callback);
	glfwSetFramebufferSizeCallback(win->window, window_resize_callback);
#endif

#if defined ANDROID && defined OPENGLES
	pe_wm_egl_init();	
#endif

  win->initialized = true;
	
	LOG("Window created\n");
}


void pe_wm_window_init(EngineWindow* window){
	LOG("Initialing Window\n");
	if(window->init != NULL)
		window->init();
	window->initialized = true;

	LOG("Window init pe_wm_window_init\n");
}



void window_set_focus(EngineWindow* window){
    current_window->focus = false;
#ifdef DESKTOP
    glfwShowWindow(window->window);
    glfwFocusWindow(window->window);
    //memset(&input,0,sizeof(Input));
    glfwMakeContextCurrent(window->window);
#endif
    window->focus = true;
    current_window = window;
    //LOG("Focus windows change\n");
}

void pe_wm_context_current(){

#ifdef LINUX
     glfwMakeContextCurrent(current_window->window);
#endif
#ifdef ANDROID
      pe_wm_egl_context_make_current();
#endif

}

void pe_wm_swap_buffers() {
#ifdef ANDROID
  eglSwapBuffers(display, surface);
#endif

#ifdef LINUX
  glfwSwapBuffers(current_window->window);

#endif
}

bool is_wm_swapped(){
	if(pe_wm_swapped){
		pe_wm_swapped=false;
	}
	return true;
}

void pe_wm_events_update() {
#ifdef LINUX
  glfwPollEvents();
#endif
#ifdef ANDROID
  pe_android_poll_envents();
#endif
}
void pe_wm_windows_draw() {

  for (u8 i = 0; i < engine_windows.count; i++) {
    EngineWindow *window = array_get(&engine_windows, i);
    if (pe_renderer_type == PEWMOPENGLES2) {
      pe_wm_context_current();
    }
    if (!window->initialized)
      continue;
    if (pe_wm_should_close(window)) {
      window->finish();
      LOG("Window close\n");
      continue;
    }
    window->draw();

    if (pe_renderer_type == PEWMOPENGLES2) {

      pe_wm_swap_buffers();
			pe_wm_swapped = true;
			pe_wm_events_update();

    }
  }
}
