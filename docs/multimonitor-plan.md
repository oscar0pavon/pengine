# Multimonitor swordfish — DRM mode, extended desktop

## Context

Swordfish (`/root/swordfish`) is a Wayland compositor + Vulkan 3D scene that, when launched from a TTY, renders direct-to-display through pengine's `VK_KHR_display` path (`is_drm_rendering`). Today that path drives exactly one monitor: `vk_get_displays()` (`/root/pengine/src/engine/renderer/display.c:29,41`) enumerates every display but keeps only `displays[0]`/`modes[0]`, and `pe_vk_create_display_surface()` hardcodes plane 0 and a 1920×1080 extent (`surface.c:40-44`).

Goal: **each connected monitor becomes its own wl_output with its own tiling area** (extended desktop). Static detection at startup — no hotplug. The nested-Wayland-window mode stays single-monitor.

The recent pengine work (`PRenderTarget`, HEAD 68f29fa) already made surface/swapchain/framebuffers/sync/commands/viewport per-target; there is just one instance (`main_render_target`). **Swordfish is currently out of sync with that API and does not compile against the installed headers** — syncing it is a prerequisite phase.

## Key design decisions

- **Registry**: fixed array `PRenderTarget pe_render_targets[PE_VK_MAX_RENDER_TARGETS]` (4) + `u32 pe_render_targets_count` (default 1), with `#define main_render_target (pe_render_targets[0])` so all existing consumers (model.c, engine2d.c, swordfish, chess) compile unchanged.
- **Display selection**: rewrite `display.c` — per display pick the native mode (`visibleRegion == physicalResolution`, fallback `modes[0]`); enumerate planes via `vkGetPhysicalDeviceDisplayPlanePropertiesKHR` + `vkGetDisplayPlaneSupportedDisplaysKHR` and assign each display the first unused supporting plane; extent from the chosen mode. A display with no free plane is skipped with a message.
- **Render pass stays one global** (`pe_vk_render_pass`), created from targets[0]'s format; assert all targets share the format (amdgpu offers B8G8R8A8_SRGB everywhere) and `exit(1)` with a message otherwise. Avoids per-target pipelines.
- **`pe_vk_draw_scene` widens** to `void (*)(PRenderTarget *target, VkCommandBuffer *cmd, uint32_t image_index)` — the app derives the output index as `target - pe_render_targets`.
- **UBO aliasing solved by sequential draw**: per-model uniform buffers/descriptor sets are shared across targets, but `pe_vk_draw_frame()` already blocks on the frame fence right after submit in DRM mode (`draw.c:147-149`), so target k's GPU reads finish before target k+1's CPU writes. Document with `//INFO`; multi-target is DRM-only because of this. One real fix needed: size per-model UBO/descriptor arrays by a new `pe_vk_targets_max_images_count()` instead of one target's `images_count` (another target may have more swapchain images → OOB).
- **Camera**: wire the dead `PRenderTarget.camera` via a new `camera_init_with_size(PCamera*, w, h)`; `camera_init()` becomes a wrapper reading `pe_window_width/height` (chess unchanged). `pe_window_width/height` become "target 0's real size", set from the actual extent after swapchain creation (also fixes the latent 1920×1080-vs-1916×1040 mismatch on DRM).
- **Per-output 2D**: new `pe_2d_draw_on_target(PModel*, PRenderTarget*, u32 image_index, vec2 pos, vec2 size)` in engine2d.c writing `glm_ortho(0, target->width, 0, target->heigth, …)` into the UBO projection per draw.
- **Scene split**: 3D city + system monitor + HUD draw on output 0 only; client windows tile per output; the cursor draws on whichever output contains it.
- **Virtual coordinate space**: outputs laid left-to-right (`x = sum of previous widths`, `y = 0`). Layout tiles in virtual coords, so pointer.c hit-testing works unchanged; draw subtracts the output origin.

## Phase 1 — pengine (all under `/root/pengine/src/engine/`)

1. **renderer/vulkan.h / vulkan.c / renderer.h**: registry array + count + `main_render_target` macro + `pe_vk_targets_max_images_count()`; widen `pe_vk_draw_scene` extern (forward-declare `PRenderTarget`). Delete dead globals: `viewport`/`scissor` (vulkan.c:53-54), `pe_vk_color_image`/`pe_vk_color_memory` (surface.c:15-16).
2. **renderer/display.c/.h**: replace `vk_display`/`vk_display_mode` globals with
   ```c
   typedef struct PVkDisplay { VkDisplayKHR display; VkDisplayModeKHR mode;
     VkExtent2D extent; u32 plane_index; u32 plane_stack_index; } PVkDisplay;
   extern PVkDisplay pe_vk_displays[PE_VK_MAX_RENDER_TARGETS];
   extern u32 pe_vk_displays_count;
   ```
   Mode + plane selection per the design decision; print each display's chosen mode and plane.
3. **renderer/surface.c/.h**: `pe_vk_create_surface(target, u32 display_index)` (Wayland path ignores index); display surface fills displayMode/planeIndex/planeStackIndex/imageExtent from `pe_vk_displays[i]`, sets `target->width/heigth`.
4. **renderer/swap_chain.c**: DRM extent fallback uses `target->width/heigth` instead of `{1920,1080}` (swap_chain.c:88-94); `pe_vk_create_swapchain()` writes the final extent back into `target->width/heigth`. Delete dead `pe_vk_swapchain_image_count`/`pe_vk_swch_images` and `pe_vk_create_exportable_images()` (vk_images.c:482-497 — no callers; kills the loop-on-zero bug).
5. **renderer/vulkan.c `pe_vk_init()`**: after device setup, DRM sets `pe_render_targets_count = pe_vk_displays_count`; loop targets for surface/swapchain/viewport/image-views; set `pe_window_width/height` from targets[0]; format-equality assert; render pass + layouts + pipelines + upload pool once; loop targets for command pool/color/depth/framebuffers/commands/semaphores + `camera_init_with_size(&t->camera, extent)`. `pe_vk_end()` loops targets for per-target teardown.
6. **renderer/render_thread.c `pe_frame_draw()`**: loop `pe_vk_draw_frame(&pe_render_targets[i])` with the `//INFO` about the DRM fence wait making sequential UBO rewrites safe.
7. **renderer/draw.c**: hook definition + call gain the `target` param (draw.c:20, 85-86).
8. **renderer/queues.c**: `q_present_family = q_graphic_family;` with `//INFO` (currently uninitialized; the surface-support check is commented out at queues.c:36-44).
9. **renderer/uniform_buffer.c + descriptor_set.c**: size per-model arrays/pools by `pe_vk_targets_max_images_count()`.
10. **camera.c/.h**: `camera_init_with_size()`; **engine2d.c/.h**: `pe_2d_draw_on_target()`.
11. **sync.c/commands.c**: delete dead globals (sync.c:8-13 + sync.h externs, commands.c:11).
12. **demos/chess/chess.c**: `chess_draw_scene` gains unused `PRenderTarget*` first param (chess.c:193).

Checkpoint: `cd /root/pengine && make -j24 && make install`, plus build chess.

## Phase 2 — swordfish API sync (single-output preserved; `/root/swordfish/source_code/`)

- **main.c**: call `pe_frame_draw()` instead of `pe_vk_draw_frame()` (main.c:131); move `camera_init(&main_camera)` after `pe_vk_init()`.
- **swordfish.c**: `swordfish_draw_scene(PRenderTarget *target, …)`; thread target into `processes_draw`/`system_monitor_draw`/`draw_surfaces`/`hud_draw`/`cursor_draw`; replace the dead `pe_vk_fence_in_flight` wait (swordfish.c:107-110) with a loop over targets waiting `target->fence_in_flight`; add `&main_render_target` at swordfish.c:67.
- **processes.c:433-437, city.c:313-317, system_monitor.c:535-539, cursor.c:80-84, hud.c:210**: add `&main_render_target` to the descriptor/uniform create/update calls.

Checkpoint: `make -C /root/swordfish/source_code` (read the warnings — `-Wno-incompatible-pointer-types` hides drift); run nested in a Wayland session, identical behavior.

## Phase 3 — swordfish multi-output

- **New `outputs.c`/`outputs.h`**: `SwordfishOutput { x, y, width, height, name }` table filled from `pe_render_targets` in `main()` right after `pe_vk_init()` (before compositor thread — no locking needed); helpers `swordfish_output_at(x)`, `swordfish_output_index_at(x)`, `swordfish_virtual_width()`. Wayland-window mode yields one output naturally.
- **compositor/output.c**: one `wl_global` per output with `data = &swordfish_outputs[i]`; `send_output_state()` sends per-output geometry x, mode, name `"swordfish-N"`; `output_send_surface_enter()` gains an output index and matches client + output.
- **compositor/tasks.h**: `Task` gains `int output_index`.
- **compositor/top_level.c** (map path): assign `output_index = swordfish_output_index_at(cursor_x)`; send enter for that output.
- **compositor/layout.c**: loop outputs; per-output free rect `{out->x + LAYOUT_HALF_GAP, LAYOUT_HALF_GAP, out->width - LAYOUT_GAP, out->height - LAYOUT_GAP}` in virtual coords; tiling filters on `task->output_index`. Optional: a keybinding to move the focused window to the next output.
- **mouse.c**: clamp x to `swordfish_virtual_width()-1`, y to the current output's height-1; absolute-motion scaling uses virtual dimensions.
- **swordfish.c**: `out = target - pe_render_targets`; city/monitor/HUD only when `out == 0`; `draw_surfaces` skips tasks on other outputs and positions quads at `tile_x - out->x` via `pe_2d_draw_on_target`; `cursor_draw` only on the cursor's output, origin-adjusted; hud.c/cursor.c switch to `pe_2d_draw_on_target`.
- `WINDOW_WIDTH/HEIGHT` remain only as the nested-window size.

## Verification

Every pengine change: `cd /root/pengine && make -j24 && make install`, then `make -C /root/swordfish/source_code`.

Nested regression: run swordfish inside a Wayland session — one window, tiling/cursor/clients as before.

TTY run with 2 monitors:
- Logs: `Vulkan displays count: 2`, per-display native mode + plane prints, two swapchain extents matching each monitor, no `Can't create display surface`.
- Both monitors light up; output 0 shows city + monitor + HUD, output 1 dark until a client maps.
- Launch a client (`WAYLAND_DISPLAY=… foot`) with the cursor on monitor 2 → tiles on monitor 2; cursor crosses the boundary continuously; click/typing focus works on both outputs; clients see two wl_outputs with x-offset geometry and get `wl_surface.enter` for the right one.

## Risks

- **Plane availability on amdgpu**: if there are fewer free supporting planes than displays, the extra display is skipped (reduced count), not a crash. If surface creation fails, check `vkGetDisplayPlaneCapabilitiesKHR` next.
- **Frame pacing**: sequential FIFO presents + the DRM fence wait + `usleep(16667)` can beat against two vblank clocks (up to ~half rate). Acceptable for v1; later fix is per-target pacing.
- **Enumeration order ≠ physical left-to-right order**: monitors may be "swapped"; a config/env override is a cheap follow-up.
- **Format mismatch across monitors**: init exits with a message; near-impossible on one amdgpu card.
