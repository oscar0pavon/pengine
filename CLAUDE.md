# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

Pavon Engine — a Vulkan/OpenGL-ES2 game engine written in C for GNU/Linux (plus an Android
target). The build produces a **static library** `lib/libpengine.a`; there is no engine
executable. Games and the editor are separate programs that link against it (see `demos/`,
and the out-of-tree consumer the recent history calls *swordfish*).

## Build

```
make -j24              # engine + shaders -> lib/libpengine.a, src/shaders/*.spv
make clean
make install           # headers -> /usr/local/include/pengine, lib -> /usr/local/lib
make LOG=1             # verbose: the Makefiles do `$(LOG).SILENT:`, so any value of LOG
                       # turns the target into `1.SILENT` and command echo comes back
```

There are no tests and no lint target. "Does it build" is the only automated check, so
after touching anything under `src/engine` run a `make clean && make -j24` — incremental
builds miss header-only breakage because there are no dependency files.

Requires: `cc`, `glslc`, freetype, vulkan, X11, GL/EGL, wayland, and two of the author's
own libraries installed under `/usr/local`: **cglm** (headers only, `-I/usr/local/include`)
and **pway** (the Wayland windowing lib, `libpway.a`).

`compile_commands.json` comes from `./generate_compile_commands.sh` (needs `jq`); note the
script hardcodes `/home/pavon/pengine/src/engine` as the directory — fix it locally if you
regenerate, don't commit the change unless asked.

Sub-builds: `make -C src/engine WORKDIR=$(pwd)` compiles the library only,
`make -C src/shaders` runs `glslc` over every `.vert`/`.frag`.

`ar` names archive members by basename, and the tree has two `shader.c`
(`src/engine/shader.c` and `src/engine/renderer/opengl/shader.c`). The archive
rule therefore deletes and rebuilds `libpengine.a` with `ar rcsP` so member names
keep their paths — without `P` one of the two silently loses and edits to it stop
reaching the library. Keep that in mind before adding a source whose basename
already exists elsewhere in `src/engine`.

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
  `VULKAN`/`OPENGL_ES2`/`DESKTOP`/`EDITOR`, which gate struct members and includes.
- `CFLAGS` matters for consumers too, not just the engine: the headers do not compile
  cleanly without `-Wno-incompatible-pointer-types` (the engine passes `&main_camera`, a
  `CameraComponent`, to `camera_init(Camera*)`) or without `-fcommon`.
- `LIBRARIES` carries `-lpway` and `-llodepng` alongside the system libraries, because
  `libpengine.a` calls into both.

`src/engine/files.h` is **generated** by `scripts/create_engine_file_h.sh` and gitignored.
It bakes absolute paths to shaders, editor gizmo models and fonts from the repo checkout,
so a built library only finds its content where it was built. Adding a new asset the engine
loads by name means adding a `#define file_*` line to that script.

## Architecture

### Program shape

An application fills a `PGame` (`src/engine/game.h`) with `init`/`update`/`draw`/`input`
callbacks and calls `pengine_run(&game)`. That runs `pe_main_loop()` in `src/engine/main.c`:
`pe_init()` → `pe_render_thread_init()` → `game->init()` → spawn the input thread → loop of
`game->update()`, `pe_frame_draw()`, `game->draw()`.

`pe_init()` (`base.c`) order matters and is load-bearing: `pe_init_memory()` first, then
arrays, then globals, then window creation, then the render thread. Nothing that allocates
may run before `pe_init_memory()`.

Threads: main, render, input, and (stubbed) audio/collision. Anything touching GL/Vulkan
state must run on the render thread — cross-thread work is queued as `PEThreadCommand`s via
`pe_th_exec_in()` / `pe_th_exec_function()` and drained by `pe_thread_control()`.

### Memory

`memory.c` allocates one ~750 MB block up front (`INIT_MEMORY`) and hands out stack/pool
slices (`allocate_memory`, `allocate_stack_memory`, marker-based rewind). There is no
`free()` for individual objects. `Array` (`array.h`) is the one container used everywhere;
it lives in that arena and stores either values or pointers (`isPointerToPointer`).

### Scene model

Game objects are **Elements** (`elements.h`): an id, a name, a `TransformComponent`, and an
`Array` of `ComponentDefinition` (type tag + `void* data`). Component types are the
`ComponentType` enum in `components/components.h` — extending the engine with a new kind of
object means a new enum value plus handling in the per-frame walks, not a new struct type.

Per frame the engine walks components to fill `models_for_test_occlusion`, runs occlusion /
distance tests, and fills `frame_draw_static_elements` and `frame_draw_skinned_elements`,
which is what the renderer actually draws. Loaded models land in `array_models_loaded`.
`docs/code.txt` describes this flow in the author's words and is worth reading.

Most engine state is **global**, declared as tentative definitions in headers and made to
link by `-fcommon` (see `CFLAGS`). Recent work has been converting these to a real
`extern` + one definition in a `.c`; prefer that for anything new, since a consumer built
without `-fcommon` cannot link the tentative-definition form.

### Renderer

`src/engine/renderer/` is the Vulkan backend, one file per Vulkan object
(`instance`, `physical_devices`, `logical_device`, `surface`, `swap_chain`, `render_pass`,
`pipeline`, `descriptor_set`, `commands`, `sync`, …). `renderer/opengl/` is a second,
older ES2 backend. Which one runs is `pe_renderer_type` (`PEWMVULKAN` / `PEWMOPENGLES2`),
checked at runtime in `render_thread.c` and the frame path.

The renderer must not know about any particular application. The seams it exposes
(`renderer/renderer.h`) are:

- `pe_vk_draw_scene` — function pointer the application sets; the render pass calls it if
  it is non-NULL. Do not call an application's draw function by name from here.
- `is_drm_rendering`, `is_wayland_window` — declared extern in `renderer.h`, defined in
  `vulkan.c`.
- `pe_window_width` / `pe_window_height` — the size everything renders at; the swap chain
  extent, the camera and the 2D ortho projection all read them, and an application sets
  both before `pe_vk_init()`. There is no fixed-resolution macro.

`-Wno-implicit-function-declaration` is on, so a call to a function that does not exist
compiles and only fails at link (or, historically, didn't fail at all). Be suspicious: this
is how `main.c` came to call a `time_start()` nobody defined, and how `base.c` called
`pway_create_window()` with one argument instead of three.

The ES2 backend has three conventions that are easy to break by accident:

- GLSL ES 100 has no location qualifier, so `create_engine_shader()` calls
  `glBindAttribLocation` for `vPosition`/`vUV`/`vColor`/`vNormal` before linking. The
  numbers `update_draw_vertices()` feeds must match those bindings; leaving it to the
  driver shifts them whenever a shader stops using one of the attributes.
- `CGLM_FORCE_LEFT_HANDED` mirrors handedness, so front faces come out clockwise on screen
  and the GL init sets `glFrontFace(GL_CW)`.
- The `projection[1][1] *= -1` in `camera_init()` is Vulkan's +Y-down clip space and is
  applied only when `pe_renderer_type == PEWMVULKAN`.

The model loader fills a `PModel`'s own `vertex_array`/`index_array`; `PModel.mesh` is a
separate view (buffer ids plus counts) that the draw path reads and that callers copy
between models to share geometry. Whoever loads a model publishes it —
`add_element_with_model_path()` is the example. glTF makes `NORMAL` optional, so the loader
generates flat normals when a primitive has none.

### Platforms

`src/engine/platforms/linux` and `.../android`. Android has its own `CMakeLists.txt`,
manifest and `main.c` under `platforms/android/`, and is built out of this Makefile tree;
`docs/android` and `docs/compile` cover APK packaging and cross-compiled freetype.

## Conventions

- Engine symbols are prefixed `pe_` (`pe_vk_` for Vulkan, `pe_wm_` for window manager,
  `pe_th_` for threads); types are `P`-prefixed (`PModel`, `PMesh`, `PMaterial`, `PGame`).
- Includes inside `src/engine` use the `engine/...` / `renderer/...` form so the same
  headers resolve after `make install` under `/usr/local/include/pengine`.
- Comments are sparse. Where one exists it usually starts `//INFO` and explains *why* a
  non-obvious constraint holds — match that style rather than narrating what the code does.
- Commit messages are lowercase imperative summaries with a prose body explaining the
  mechanism and consequence of the bug or change, not a bullet list of edits.
