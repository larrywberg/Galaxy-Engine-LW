# AI Status

This is the handoff doc. Read this file first when starting work in the repo.

## Project Overview

- Galaxy-Engine is a C++ galaxy/physics simulator built on raylib with an ImGui-based desktop UI.
- The web build targets Emscripten/WebAssembly; the browser UI is a React overlay in `web/ui.js`.
- Web UI talks to the engine via exported C API functions in `GalaxyEngine/src/web_api.cpp`.
- Core physics includes Barnes-Hut quadtree gravity, SPH fluids, constraints, and optics/light simulation.

## Build/Run (Web/WASM)

- `npm run clean` removes `build/wasm-*` and `build/emscripten_cache`.
- `npm run dev` configures + builds `wasm-debug` and serves it.
- `npm run dev:build-run` rebuilds `wasm-debug` and serves it (no reconfigure).
- `npm run release` configures + builds `wasm-release` and serves it.
- Outputs: `build/wasm-debug` or `build/wasm-release`.
- Server: `scripts/serve-wasm.js` with COOP/COEP headers and `Cache-Control: no-store` by default.
- Emscripten cache lives at `build/emscripten_cache` (via `EM_CACHE`).

## UI Surfaces

- Browser UI: React overlay in `web/index.html` + `web/ui.js` (this is what you see in the screenshot).
- Desktop UI: ImGui panel in `GalaxyEngine/src/UI/UI.cpp` (not visible in web builds).
- Web UI state persists in `localStorage`.

## Current Web UI Coverage (High-Level)

- Visuals: color modes, glow, trails (length/thickness/toggles), particle size multiplier, gravity field display controls.
- Physics: threads amount, theta, domain size, time scale, symplectic integrator, softening, gravity strength.
- Physics (new): Gravity Ramp (toggle + start/seconds) and Velocity Damping (toggle + rate).
- Simulation toggles: dark matter, looping space, fluid ground mode, temperature, highlight selected.
- Temperature, constraints, and fluids/SPH sliders.
- Tools: particle spawn tools + brush tools; optics tools (lights, walls, lenses).
- Actions: selection utilities, pin/unpin, center camera.
- Stats: fps, frame time, particle counts, lighting counts.
- Recording + sound tabs are present in the React UI.

## Recent Engine/Web Changes

- Gravity ramp + velocity damping implemented in C++ engine and exposed to web UI.
- Thread pool capped to 4 for wasm; OpenMP removed for wasm path.
- Web server and `.htaccess` disable asset caching to force full reloads.
- Pointer leave/enter hides the brush cursor in the web build.

## Known Gaps vs Desktop UI

- GPU gravity toggle and some native-only actions (right-click menu, extra export tools) are not exposed in the React UI.
- If in doubt, compare `web/ui.js` to `GalaxyEngine/src/UI/UI.cpp`.
