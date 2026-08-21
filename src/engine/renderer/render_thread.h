#ifndef ENGINE_RENDER_THREAD
#define ENGINE_RENDER_THREAD
#include <engine/array.h>
#include <engine/threads.h>

typedef struct PRenderThreadDefinition{
	void(*init)(void);
	void(*draw)(void);
	void(*end)(void);
}PRenderThreadDefinition;

typedef struct RenderThread{
	void(*draw)(void);
	void(*init)(void);
	void(*finish)(void);
}PERenderThread;

//INFO pe_main_loop() calls this as the step after pe_init() and had no
//declaration for it, so it was reading the return value of an int returning
//function that returns nothing
void pe_render_thread_init();

void pe_render_thread_start_and_draw();

void pe_frame_clean();

void pe_frame_draw();

PEThread thread_render;

Array array_render_thread_init_commmands;
Array array_render_thread_commands;

PRenderThreadDefinition render_thread_definition;

#endif
