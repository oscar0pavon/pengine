#include "windows_manager.h"
#include <stdio.h>

#include <engine/game.h>
#include <engine/log.h>
#include <engine/text_renderer.h>

#include <X11/Xlib.h>
#include <GL/glx.h>

Display                 *display;
Window                  root_window;
GLint                   attributes[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
XVisualInfo             *visual_info;
Colormap                color_map;
XSetWindowAttributes    set_window_attributes;
Window                  window;
GLXContext              gl_context;
XWindowAttributes       window_attributes;

void window_manager_error_callback(int error, const char* description)
{
      printf("GLFW error: %s\n",description);
}

void pe_wm_glfw_init(){
  
    if (pe_renderer_type == PEWMOPENGLES2) {
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
        glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	    LOG("Windows manager initialized in OPENGL\n");
    } else if (pe_renderer_type == PEWMVULKAN) {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	    LOG("Window Manager in VULKAN\n");
    }
	//MSAA
	glfwWindowHint(GLFW_SAMPLES,16);

    if (pe_renderer_type == PEWMOPENGLES2) {

    }

    glfwSetErrorCallback(window_manager_error_callback);
    glfwInit();

    LOG("GLFW initialized\n");

}



void windows_manager_init(){
  LOG("Window manager init....\n");
	pe_wm_glfw_init();		
  LOG("Window manager initilized\n");
  pe_is_window_init = true;
}

void pe_wm_make_context(EngineWindow*window){
  glfwMakeContextCurrent(window->window);
}

void pe_wm_create_x11_window(){

    display = XOpenDisplay(NULL);

    visual_info = glXChooseVisual(display,0,attributes);
    
    root_window = XDefaultRootWindow(display);
    
    color_map = XCreateColormap(display, root_window, visual_info->visual, AllocNone);

    set_window_attributes.colormap = color_map;
    set_window_attributes.event_mask = ExposureMask | KeyPressMask;
    
    window = XCreateWindow(display, root_window, 0, 0, 
        800, 600, 0,
        visual_info->depth, InputOutput, visual_info->visual,
        CWColormap | CWEventMask, &set_window_attributes);
    
    XStoreName(display, window, "pengine");
   
    XMapWindow(display, window);
    XFlush(display);

    while(1){

    }

}

void pe_wm_create_window(EngineWindow *win, EngineWindow *share_window,
                   const char *name) {
  if (win == NULL) {
            LOG("ERROR: Window not found\n");
            return;
  }
  if (win->initialized)
            return;

  current_window = win;

  GLFWwindow *share_glfw_window = NULL;
  if (share_window)
            share_glfw_window = share_window->window;

  if (pe_renderer_type == PEWMVULKAN) {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  }

  GLFWwindow *new_window = glfwCreateWindow(
      INIT_WINDOW_SIZE_X, INIT_WINDOW_SIZE_Y, name, NULL, share_glfw_window);
  if (!new_window) {
            LOG("Window can't be created\nPavon Engine was closed\n");
            exit(-1);
  }
  win->window = new_window;

  if (pe_renderer_type == PEWMOPENGLES2) {
            glfwMakeContextCurrent(win->window);
  }

  glfwSetWindowUserPointer(win->window, win);

  camera_heigth_screen = INIT_WINDOW_SIZE_Y;
  camera_width_screen = INIT_WINDOW_SIZE_X;
  window_update_viewport(INIT_WINDOW_SIZE_X, INIT_WINDOW_SIZE_Y);


  win->initialized = true;
  pe_is_glfw_window_created = true;
}

void window_resize_callback(GLFWwindow* window, int width, int height){
    camera_heigth_screen = height;
    camera_width_screen = width;
		window_set_focus(current_window); 
		window_update_viewport(width,height);
}

void window_focus_callback(GLFWwindow* window,int is_focus){
    EngineWindow* editor_window = glfwGetWindowUserPointer(window);
    if(is_focus == GLFW_TRUE){
        editor_window->focus = true;
    }
    if(is_focus == GLFW_FALSE){
       //editor_window->focus = false;
    }
}




void window_initialize_windows(){
	for(u8 i = 0; i<engine_windows.count ; i++ ){
		EngineWindow* window = array_get(&engine_windows,i);
		if(window->initialized)
			   continue;
		window->init();	
	}
}


