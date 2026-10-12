# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

Pavon Engine — a Vulkan game engine written in C for GNU/Linux. The build produces a
**static library** `lib/libpengine.a`; there is no engine executable. Games and the editor
are separate programs that link against it (see `demos/chess`, and the out-of-tree consumer
the recent history calls *swordfish*).

Vulkan is the only backend. An OpenGL ES2 backend, the X11 window/input path and the
Element/Component scene graph used to live here and were removed; nothing in the tree
should grow a `gl*` call, an `X*` call or a component type again.

## Build

```
make -j24              # engine + shaders -> lib/libpengine.a, src/shaders/*.spv
make clean
make install           # headers -> /usr/local/include/pengine, lib -> /usr/local/lib
make LOG=1             # verbose: the Makefiles do `$(LOG).SILENT:`, so any value of LOG
                       # turns the target into `1.SILENT` and command echo comes back
```

There are no tests and no lint target. "Does it build" is the only automated check.
Incremental builds are trustworthy for header changes: the compile rules pass `-MMD -MP`
and the engine Makefile `-include`s the resulting `.d` files, so editing a header rebuilds
every object that read it. A `make clean` is still the honest check before claiming a
change builds, since nothing tracks the Makefiles themselves.

Requires: `cc`, `glslc`, `git`, `curl`, vulkan, wayland, and the author's **pway** (the
Wayland windowing lib, `libpway.a`) installed under `/usr/local`. **cglm** (headers only)
is taken from the host, or fetched into `src/external` by `./configure` when the
compiler does not find it. `-lEGL`/`-lwayland-egl` are still on the link line
because `libpway.a`'s `pway.o` references its own EGL setup, not because the engine draws
with GL.

`compile_commands.json` comes from `./generate_compile_commands.sh` (needs `jq`); note the
script hardcodes `/home/pavon/pengine/src/engine` as the directory — fix it locally if you
regenerate, don't commit the change unless asked.

Sub-builds: `make -C src/engine WORKDIR=$(pwd)` compiles the library only,
`make -C src/shaders` runs `glslc` over every `.vert`/`.frag`.

`ar` names archive members by basename, so two sources sharing one basename would
both claim the same member and one would silently lose — edits to it stopping at the
library. The archive rule therefore deletes and rebuilds `libpengine.a` with `ar rcsP`
so member names keep their paths. Keep that in mind before adding a source whose
basename already exists elsewhere in `src/engine`.

## Building something against the engine

`demos/chess/Makefile` is the working model — `make -C demos/chess` builds `pchess`, run it
from its own directory since it loads its `.glb` files by relative path. The pattern:

```
$(CC) $(CFLAGS) $(GLOBAL_DEFINE) $(CINCLUDES) app.c -L<repo>/lib -lpengine $(LIBRARIES) -o app
```

All four variables come from `include.make` and none of them is decoration:

- `GLOBAL_DEFINE` is **not optional**. `-DCGLM_FORCE_DEPTH_ZERO_TO_ONE` and
  `-DCGLM_FORCE_LEFT_HANDED` change cglm's projection matrices; a consumer compiled without
  them gets silently different math than the engine it links to. The same goes for
  `VULKAN`/`DESKTOP`/`EDITOR`, which gate struct members and includes.
- `CFLAGS` matters for consumers too, not just the engine: the headers still declare most
  globals as tentative definitions, so `-fcommon` is required. It is the only flag left in
  `CFLAGS` besides `-g -fPIC`; the five `-Wno-` suppressions are gone and the tree builds
  clean without them.
- `LIBRARIES` carries `-lpway` alongside the system libraries, because `libpengine.a`
  calls into it. lodepng is not linked: `./configure` fetches it into `src/external` and
  the library compiles it in, like parson.

`src/engine/files.h` is **generated** by `scripts/create_engine_file_h.sh` and gitignored.
It bakes absolute paths to `.spv` shaders and editor gizmo models from the repo checkout,
so a built library only finds its content where it was built. Adding a new asset the engine
loads by name means adding a `#define file_*` line to that script. Most of the gizmo entries
have no in-tree user — they are there for the out-of-tree editor, so do not prune them just
because nothing in this repo names them.

## Architecture

### Program shape

An application fills a `PGame` (`src/engine/game.h`) with `init`/`update`/`draw`/`input`
callbacks and calls `pengine_run(&game)`. That runs `pe_main_loop()` in `src/engine/main.c`:
`pe_init()` → `pe_render_thread_init()` → `game->init()` → spawn the input thread → loop of
`game->update()`, `pe_frame_draw()`, `game->draw()`.

`pe_init()` (`base.c`) order matters and is load-bearing: `pe_init_memory()` first, then
arrays, then globals, then window creation, then the render thread. Nothing that allocates
may run before `pe_init_memory()`. Because `pe_render_thread_init()` (and so `pe_vk_init()`)
runs before `game->init()`, an application's `init` can already assume a device, a swap
chain, the descriptor set layouts and the pipeline layouts exist.

Threads: main, render, input, and (stubbed) audio/collision. Anything touching GL/Vulkan
state must run on the render thread — cross-thread work is queued as `PEThreadCommand`s via
`pe_th_exec_in()` / `pe_th_exec_function()` and drained by `pe_thread_control()`.

### Memory

`memory.c` allocates one ~750 MB block up front (`INIT_MEMORY`) and hands out stack/pool
slices (`allocate_memory`, `allocate_stack_memory`, marker-based rewind). There is no
`free()` for individual objects. `Array` (`array.h`) is the one container used everywhere;
it lives in that arena and stores either values or pointers (`isPointerToPointer`).

The `count` passed to `array_init()` is a starting capacity, not a limit — `array_add()`
grows past it by taking a bigger block from the arena and copying into it. Because the
arena has no `free()`, the old block stays allocated, so a good initial guess still saves
memory. **Growing moves the elements**: a pointer from `array_get()` or `array_get_last()`
must not be held across an `array_add()` on the same array.

### Scene model

**The engine has no scene.** It owns models, not a world made of them. `pe_frame_draw()` is
just `pe_vk_draw_frame()`, and the application's scene is reached only through the
`pe_vk_draw_scene` hook the render pass calls. An application keeps its own objects in
whatever shape suits it and records their draws from that callback — `demos/chess` is the
worked example, an array of `ChessPiece` each holding a `PModel` and a colour.

`docs/code.txt` describes an older Element/Component flow that no longer exists.

Most engine state is **global**, declared as tentative definitions in headers and made to
link by `-fcommon` (see `CFLAGS`). Recent work has been converting these to a real
`extern` + one definition in a `.c`; prefer that for anything new, since a consumer built
without `-fcommon` cannot link the tentative-definition form.

### Renderer

`src/engine/renderer/` is the Vulkan backend, one file per Vulkan object
(`instance`, `physical_devices`, `logical_device`, `surface`, `swap_chain`, `render_pass`,
`pipeline`, `descriptor_set`, `commands`, `sync`, …). The window is a Wayland toplevel
unless the application sets `is_drm_rendering` before `pengine_run()`, which drives KMS
through `VK_KHR_display` instead.

The renderer must not know about any particular application. The seams it exposes
(`renderer/renderer.h`) are:

- `pe_vk_draw_scene` — function pointer the application sets; the render pass calls it if
  it is non-NULL. Do not call an application's draw function by name from here.
- `is_drm_rendering` — declared extern in `renderer.h`, defined in `vulkan.c`. False by
  default, which is the Wayland window path.
- `pe_window_width` / `pe_window_height` — the size everything renders at; the swap chain
  extent, the camera and the 2D ortho projection all read them, and an application sets
  both before `pe_vk_init()`. There is no fixed-resolution macro.

**Resizing** is `pe_vk_recreate_swapchain()` (`swap_chain.c`). It waits for the device to
go idle and rebuilds everything a target sizes from its extent — swap chain, image views,
colour and depth attachments, framebuffers, per-image command buffers and semaphores, and
the target's own camera. The render pass, the pipelines and the layouts survive: the format
does not change, and the viewport and scissor are dynamic state that `pe_vk_draw_commands()`
sets from the target every frame. A Wayland surface leaves the extent to the client, so
`pe_window_width`/`pe_window_height` must already hold the new size before the call.

Because it waits for idle, it belongs between two frames on the thread that draws, never in
a window event callback. `pe_vk_draw_frame()` also calls it itself when the acquire returns
`VK_ERROR_OUT_OF_DATE_KHR` (that frame is dropped) or the present comes back out of date or
suboptimal.

The one thing a rebuild cannot fix up is a model's uniform buffers and descriptor sets:
there is one per swap chain image, sized when the model was created. If the new chain has
more images than the old one, `image_index` runs off the end of both — the recreate logs
that rather than papering over it.

Undeclared calls are errors again. `-Wno-implicit-function-declaration` used to be on, so a
call to a function that does not exist compiled and only failed at link — and because the
product is a static library, an unresolved call does not fail at `ar` time either, so it
surfaced in a consumer or not at all. That is how `main.c` came to call a `time_start()`
nobody defined, how `base.c` called `pway_create_window()` with one argument instead of
three, and how `content_manager.c` went on calling two functions that left with the
element/component scene graph. Keep it that way: a new call needs a declaration in scope,
which usually means the header the function belongs to, not a local prototype.

`camera_init()` applies `projection[1][1] *= -1` for Vulkan's +Y-down clip space, and so
does `camera_update_aspect_ratio()`, which is the one to call after a resize — it reads
`camera_width_screen`/`camera_height_screen`, so those go first.

A model carries its own transform in `PModel.model_mat`, built with the `pe_model_*`
helpers in `model.c` (`pe_model_transform` composes translate * rotate * scale;
`pe_model_set_position` replaces just the fourth column). An application copies that
matrix into `uniform_buffer_object.model` before it draws.

A model is not drawable until it has descriptor sets pointing at its uniform buffers —
`pe_vk_draw_model()` binds `descriptor_sets[image_index]` on every draw. `pe_vk_load_model()`
does that. To place the same geometry more than once, use `pe_vk_model_instance()`: it
shares the vertex and index buffers but gives the copy its own uniform buffers and
descriptor sets, because those carry the per-instance model matrix.

The model loader fills a `PModel`'s own `vertex_array`/`index_array`; `PModel.mesh` is a
separate view (buffer ids plus counts) that the draw path reads and that callers copy
between models to share geometry. Whoever loads a model publishes it —
`add_element_with_model_path()` is the example. glTF makes `NORMAL` optional, so the loader
generates flat normals when a primitive has none.

Only the first UV set (`TEXCOORD_0`) and the first colour set (`COLOR_0`) are read:
Blender writes every UV map of a mesh, and a later one read over the first leaves
the model sampling a single point. `COLOR_0` lands in `PVertex.color`, alpha dropped.

Images: `image_load_from_memory()` tries lodepng (png) and falls back to
stb_image for jpeg. `pe_load_hdr_image()` / `pe_texture_from_hdr_image()` read a
Radiance `.hdr` as floats and keep it on the gpu as half float RGBA, with mipmaps;
the texture upload sizes its staging buffer by the format. Textures that are not
colour (`texture_load_from_memory_linear()`, `pe_texture_flat_normal()`) are
`R8G8B8A8_UNORM`, since the sRGB format would bend them.

Samplers filter anisotropically: `pe_vk_create_logical_device()` enables
`samplerAnisotropy` when the gpu has it and keeps the most it allows, up to 16,
in `pe_vk_max_anisotropy` (1 when it cannot, which leaves it off). It is the only
device feature enabled.

### Materials and light

A glb whose primitives each have vertices of their own loads as `PModel.parts`
(`model.h`): one index range per material, with its base colour, its metal and
roughness (glTF's roughness in green, metallic in blue) and its normal map, and
the factors of the material as a `PPartMaterial`. A part with no texture in a
slot has a white pixel, or for normals a flat one. Colour textures are read as
sRGB and the other two linear.

Parts are drawn with `pe_vk_pipeline_layout_pbr`: set 0 is the part's material
(`pe_vk_model_parts_create_descriptors()`, one set for each part and swap chain
image), set 1 the environment, and the factors go in as a push constant.
`PDrawModelCommand.environment` is that set 1. `pbr_frag.frag` is Cook-Torrance
GGX with a sun and image based light, and works the normal map's frame out from
screen derivatives, so models need no tangents.

`PEnvironment` (`environment.h`) loads a Radiance `.hdr` panorama as half float
light with mipmaps, projects it onto nine spherical harmonics for the light a
surface gets from every side, and puts the sun toward its brightest block.
Reflections read the panorama's mip levels by roughness. Nothing is prefiltered,
so a rough surface reflects a box-blurred panorama, not a true GGX lobe.
`PSky` (`sky.h`) draws the same panorama behind everything. Shaders tonemap with
ACES themselves, and write linear light since the swap chain is sRGB.

Not done: alpha blended materials (glass draws opaque), occlusion maps, shadows,
a prefiltered environment.

Parts are indexed with 32 bits, since the vertices of a car do not fit 16, and
each part has vertices of its own that are appended after the ones before it.
A file is loaded as parts as a whole: when one mesh of it is, or when it has
several meshes and no skin or geoset, every mesh is, and the textures they name
are loaded once for the file, not for each mesh.

Each part keeps a bounding sphere of its vertices, `PModelPart.bounds`, in the
model's axes, and `pe_vk_draw_model()` skips the parts whose sphere is outside the
view, tested against the camera in the model's uniform buffer object
(`frustum.h`, shared with the terrain). `PModel.view_distance` (0 for no limit) also skips the parts whose sphere is
farther from the eye than that, measured to its nearest point. `PModel.parts_drawn`
says how many the last draw sent. So a world made of many meshes in one glb is culled mesh by mesh,
and a world joined into one mesh is not.
Models whose primitives share one set of vertices, the characters told apart by
geoset, are not parts and load as they always did.

### Vehicle

`vehicle.h` is a raycast vehicle: a rigid body on springs, with no mesh and no
renderer in it. Each wheel casts a ray down the body's own up against a
`PCollisionMesh` (`terrain_collision.h`), holds the body up with a spring and
damper that has a bump stop past its travel, and pushes along the ground with a
tyre whose drive, braking and sideways grip share one limit of load times
friction. `PVehicle.steering`, `engine_force` and `brake` are what the driver
asks for; `pe_vehicle_step()` advances in steps of at most 1/240 s, and
`pe_vehicle_transform()` / `pe_vehicle_wheel_transform()` give the matrices to
draw with.

- The body's axes are Y forward, Z up and X to the **left**, which is the way the
  engine's left handed world has it. Positive steering turns toward +X.
- `stiffness` is per unit of mass the wheel carries (1/s^2), so the static sag is
  `g / stiffness`; it has to stay inside `travel` or the bump stop is what holds
  the car up.
- `rolling_resistance` (a share of the load) is what keeps a body at rest from
  drifting: hubs at different heights tilt it, and the suspension then pushes a
  little along the ground.
- Walls are a few spheres on the body pushed out of the mesh; `impact_speed` says
  how hard it hit. `physics.h` is still an empty stub; this is the only physics.

### Platforms

`src/engine/platforms/` is **not compiled by this Makefile** — no rule reaches it, and the
same is true of `audio/`, `network/` and `Math/`. `platforms/android` is a GLES/EGL port and
still calls `pe_wm_egl_end()`, which no longer exists; it needs a Vulkan port before it can
build again. `docs/android` and `docs/compile` describe the old APK packaging.

## Conventions

- Engine symbols are prefixed `pe_` (`pe_vk_` for Vulkan, `pe_wm_` for window manager,
  `pe_th_` for threads); types are `P`-prefixed (`PModel`, `PMesh`, `PMaterial`, `PGame`).
- Includes inside `src/engine` use the `engine/...` / `renderer/...` form so the same
  headers resolve after `make install` under `/usr/local/include/pengine`.
- Comments are sparse. Where one exists it usually starts `//INFO` and explains *why* a
  non-obvious constraint holds — match that style rather than narrating what the code does.
- Input keys are raw evdev codes: pway reports them, `pe_input_init()` seeds
  `pe_key_codes[]` in `Input` member order (guarded by a `_Static_assert`), and
  `pe_parse_key_event()` matches on them.
- Commit messages are lowercase imperative summaries with a prose body explaining the
  mechanism and consequence of the bug or change, not a bullet list of edits.
