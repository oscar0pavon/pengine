#ifndef ENGINE_RENDER_THREAD
#define ENGINE_RENDER_THREAD
#include "array.h"
#include "threads.h"


typedef struct RenderThread{
	void(*draw)(void);
	void(*init)(void);
	void(*finish)(void);
};

void engine_init_render();

EngineThread thread_render;

Array array_render_thread_init_commmands;
Array array_render_thread_commands;


#endif
