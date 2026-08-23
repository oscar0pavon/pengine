#ifndef RENDERER
#define RENDERER

#include <vulkan/vulkan_core.h>

#include <stdbool.h>

typedef struct PRenderTarget PRenderTarget;

//INFO the two things only the application knows: whether it drives KMS itself
//and whether it got a window from a host compositor. defined in vulkan.c so a
//header does not hand every translation unit its own copy
extern bool is_drm_rendering;
extern bool is_wayland_window;

//the application's scene. the renderer records the render pass and calls this
//in the middle of it; it used to call swordfish_draw_scene() by name, which is
//what tied the engine to one program. target is which output is being
//recorded - pe_render_targets[i] is the way to recover its index
extern void (*pe_vk_draw_scene)(PRenderTarget *target,
                                VkCommandBuffer *cmd_buffer, uint32_t index);

//optional, and only ever called on the DRM path: the application's chance to
//hand vulkan a DRM fd of its own (vkGetDrmDisplayEXT + vkAcquireDrmDisplayEXT)
//before the displays are enumerated. mesa's wsi_display otherwise scans out
//through the fd radv opened for itself, which the application has no way to
//drop - and a compositor that cannot drop DRM master cannot release the
//display when its VT is switched away. left NULL, the old behaviour stands.
//returning false is not fatal: enumeration is tried anyway
extern bool (*pe_vk_acquire_display)(void);

//the size the renderer draws at. the swap chain, the camera and the 2D
//projection all read it, and an application that wants something other than
//the default sets both before pe_vk_init()
extern uint32_t pe_window_width;
extern uint32_t pe_window_height;

#endif
