#ifndef RENDERER
#define RENDERER

#ifdef OPENGL_ES2
	#include <engine/renderer/opengl/opengl_es2.h>
#endif 

#ifdef VULKAN
	#include <engine/renderer/vulkan/vulkan.h>
#endif

#include "render_thread.h"
#include "material.h"

#endif
