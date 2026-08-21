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

Requires: `cc`, `glslc`, vulkan, wayland, and two of the author's own libraries installed
under `/usr/local`: **cglm** (headers only, `-I/usr/local/include`) and **pway** (the
Wayland windowing lib, `libpway.a`). `-lEGL`/`-lwayland-egl` are still on the link line
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
  globals as tentative definitions, so `-fcommon` is required. `-Wno-incompatible-pointer-types`
  is no longer needed to compile the headers (`main_camera` is a `PCamera` and
  `camera_init` takes one), but the engine's own build still needs it for the thread entry
  points it hands to `pthread_create`.
- `LIBRARIES` carries `-lpway` and `-llodepng` alongside the system libraries, because
  `libpengine.a` calls into both.

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
`pipeline`, `descriptor_set`, `commands`, `sync`, …). `pe_renderer_type` survives with a
single value, `PEWMVULKAN`, only so applications that set it keep compiling; nothing
branches on it.

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
`pway_create_window()` with one argument instead of three. Because the product is a static
library, an unresolved call does not even fail at `ar` time — it surfaces in a consumer.

`camera_init()` applies `projection[1][1] *= -1` for Vulkan's +Y-down clip space.

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
