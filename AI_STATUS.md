# AI Status

This file tracks what the web UI has implemented versus the native desktop app,
plus the expected build/run flows. Update it whenever UI or build flows change.

## Build/Run Order

Dev (first-time or after `wasm:clean`):
1) `npm run wasm:reconfigure`
2) `npm run wasm:build`
3) `npm run wasm:run`
Or use: `npm run wasm:rebuild-run`

Dev (incremental rebuild):
1) `npm run wasm:configure`
2) `npm run wasm:build`
3) `npm run wasm:run`
Or use: `npm run dev`

Release (first-time or after `wasm:clean`):
1) `npm run wasm:reconfigure:release`
2) `npm run wasm:build:release`
3) `npm run wasm:run:release`
Or use: `npm run wasm:rebuild-run:release`

Release (incremental rebuild):
1) `npm run wasm:configure:release`
2) `npm run wasm:build:release`
3) `npm run wasm:run:release`
Or use: `npm run release`

Notes:
- `wasm:configure` uses FetchContent in fully disconnected mode; it will fail
  if `build/` was wiped. Use `wasm:reconfigure` after a clean.
- All CMake steps use `EM_CACHE=$PWD/build/emscripten_cache`.

## Implemented In Web UI

Layout/UI shell:
- Three-column layout in `web/index.html` with a GL canvas in the middle.
- React UI overlay in `web/ui.js` (Parameters/Tools/Actions/Settings/Stats + Controls/Info panels).
- UI state is persisted to `localStorage` (sliders, toggles, panel states, tabs).

Parameters panel:
- Visuals: color mode, glow toggle, trails toggles, trails length/thickness.
- Visuals: particle size multiplier (sharpness control).
- Physics: theta and softening sliders (gravity quality/speed tradeoff).
- Recording: pause after recording, clean scene after recording, time limit, WebM capture.
- Other tabs (Advanced Stats/Optics/Sound) exist but are not wired yet.

Tools panel:
- Particle tools: draw particles, black hole, big/small galaxy, star, big bang.
- Brush tools: eraser, radial force, spin, grab.
- Optics tools: point/area/cone light, wall, circle, draw shape, lens,
  move/erase/select optics.

Actions panel:
- Selection actions: subdivide all/selected, invert/deselect, select clusters,
  delete selection/strays, pin/unpin, center camera.

Settings panel:
- Simulation: play/pause, time factor, time step.
- Physics: gravity multiplier.
- Performance: target FPS.
- Debug: draw quadtree, draw Z-curves.

Stats panel:
- Basic readout (target FPS, time factor, gravity). Native stats are not exposed.

## Not Yet Ported (Desktop Only)

- Save/Load scene, exit, fullscreen toggle.
- Multi-threading toggle and GPU (beta) toggle.
- Full parameters tree: neighbor search, system params, general physics params,
  simulation params, fluids/SPH material, fields, temperature, constraints, space
  modifiers, misc parameters.
- Optics settings beyond basic tools (light/material settings, wall/material,
  render settings).
- Full native stats panel (particles, lights, rays, selection metrics).
- Right-click menu and its actions.
- Soundtrack/sound parameter controls (beyond static links).
- Native recording/export flows (FFmpeg, screenshots, PLY/frame export).
