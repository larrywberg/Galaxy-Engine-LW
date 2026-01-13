import React, { useEffect, useMemo, useRef, useState } from "https://esm.sh/react@18";
import { createRoot } from "https://esm.sh/react-dom@18/client";
import htm from "https://esm.sh/htm@3";

const html = htm.bind(React.createElement);

const uiRoot = document.getElementById("ui-root");
const root = createRoot(uiRoot);

function useWasmApi() {
  const [api, setApi] = useState(null);

  useEffect(() => {
    window.webUiInit = () => {
      const Module = window.Module;
      if (!Module || !Module.ccall) return;

      const wrap = (name, ret, args) => (...values) => Module.ccall(name, ret, args, values);
      const apiObj = {
        setTimePlaying: wrap("web_set_time_playing", null, ["number"]),
        getTimePlaying: wrap("web_get_time_playing", "number", []),
        setTimeFactor: wrap("web_set_time_factor", null, ["number"]),
        getTimeFactor: wrap("web_get_time_factor", "number", []),
        setTargetFps: wrap("web_set_target_fps", null, ["number"]),
        getTargetFps: wrap("web_get_target_fps", "number", []),
        setGravityMultiplier: wrap("web_set_gravity_multiplier", null, ["number"]),
        getGravityMultiplier: wrap("web_get_gravity_multiplier", "number", []),
        setTimeStepMultiplier: wrap("web_set_time_step_multiplier", null, ["number"]),
        getTimeStepMultiplier: wrap("web_get_time_step_multiplier", "number", []),
        setUiHover: wrap("web_set_ui_hover", null, ["number"]),
        setToolDrawParticles: wrap("web_set_tool_draw_particles", null, ["number"]),
        getToolDrawParticles: wrap("web_get_tool_draw_particles", "number", []),
        setToolBlackHole: wrap("web_set_tool_black_hole", null, ["number"]),
        getToolBlackHole: wrap("web_get_tool_black_hole", "number", []),
        setToolBigGalaxy: wrap("web_set_tool_big_galaxy", null, ["number"]),
        getToolBigGalaxy: wrap("web_get_tool_big_galaxy", "number", []),
        setToolSmallGalaxy: wrap("web_set_tool_small_galaxy", null, ["number"]),
        getToolSmallGalaxy: wrap("web_get_tool_small_galaxy", "number", []),
        setToolStar: wrap("web_set_tool_star", null, ["number"]),
        getToolStar: wrap("web_get_tool_star", "number", []),
        setToolBigBang: wrap("web_set_tool_big_bang", null, ["number"]),
        getToolBigBang: wrap("web_get_tool_big_bang", "number", []),
        setToolPointLight: wrap("web_set_tool_point_light", null, ["number"]),
        getToolPointLight: wrap("web_get_tool_point_light", "number", []),
        setToolAreaLight: wrap("web_set_tool_area_light", null, ["number"]),
        getToolAreaLight: wrap("web_get_tool_area_light", "number", []),
        setToolConeLight: wrap("web_set_tool_cone_light", null, ["number"]),
        getToolConeLight: wrap("web_get_tool_cone_light", "number", []),
        setToolWall: wrap("web_set_tool_wall", null, ["number"]),
        getToolWall: wrap("web_get_tool_wall", "number", []),
        setToolCircle: wrap("web_set_tool_circle", null, ["number"]),
        getToolCircle: wrap("web_get_tool_circle", "number", []),
        setToolDrawShape: wrap("web_set_tool_draw_shape", null, ["number"]),
        getToolDrawShape: wrap("web_get_tool_draw_shape", "number", []),
        setToolLens: wrap("web_set_tool_lens", null, ["number"]),
        getToolLens: wrap("web_get_tool_lens", "number", []),
        setToolMoveOptics: wrap("web_set_tool_move_optics", null, ["number"]),
        getToolMoveOptics: wrap("web_get_tool_move_optics", "number", []),
        setToolEraseOptics: wrap("web_set_tool_erase_optics", null, ["number"]),
        getToolEraseOptics: wrap("web_get_tool_erase_optics", "number", []),
        setToolSelectOptics: wrap("web_set_tool_select_optics", null, ["number"]),
        getToolSelectOptics: wrap("web_get_tool_select_optics", "number", []),
        setGlowEnabled: wrap("web_set_glow_enabled", null, ["number"]),
        getGlowEnabled: wrap("web_get_glow_enabled", "number", []),
        setTrailsLength: wrap("web_set_trails_length", null, ["number"]),
        getTrailsLength: wrap("web_get_trails_length", "number", []),
        setTrailsThickness: wrap("web_set_trails_thickness", null, ["number"]),
        getTrailsThickness: wrap("web_get_trails_thickness", "number", []),
        setGlobalTrails: wrap("web_set_global_trails", null, ["number"]),
        getGlobalTrails: wrap("web_get_global_trails", "number", []),
        setSelectedTrails: wrap("web_set_selected_trails", null, ["number"]),
        getSelectedTrails: wrap("web_get_selected_trails", "number", []),
        setLocalTrails: wrap("web_set_local_trails", null, ["number"]),
        getLocalTrails: wrap("web_get_local_trails", "number", []),
        setWhiteTrails: wrap("web_set_white_trails", null, ["number"]),
        getWhiteTrails: wrap("web_get_white_trails", "number", []),
        setColorMode: wrap("web_set_color_mode", null, ["number"]),
        getColorMode: wrap("web_get_color_mode", "number", []),
        setWindowSize: wrap("web_set_window_size", null, ["number", "number"]),
        setPauseAfterRecording: wrap("web_set_pause_after_recording", null, ["number"]),
        getPauseAfterRecording: wrap("web_get_pause_after_recording", "number", []),
        setCleanSceneAfterRecording: wrap("web_set_clean_scene_after_recording", null, ["number"]),
        getCleanSceneAfterRecording: wrap("web_get_clean_scene_after_recording", "number", []),
        setRecordingTimeLimit: wrap("web_set_recording_time_limit", null, ["number"]),
        getRecordingTimeLimit: wrap("web_get_recording_time_limit", "number", []),
        clearScene: wrap("web_clear_scene", null, []),
        setToolEraser: wrap("web_set_tool_eraser", null, ["number"]),
        getToolEraser: wrap("web_get_tool_eraser", "number", []),
        setToolRadialForce: wrap("web_set_tool_radial_force", null, ["number"]),
        getToolRadialForce: wrap("web_get_tool_radial_force", "number", []),
        setToolSpin: wrap("web_set_tool_spin", null, ["number"]),
        getToolSpin: wrap("web_get_tool_spin", "number", []),
        setToolGrab: wrap("web_set_tool_grab", null, ["number"]),
        getToolGrab: wrap("web_get_tool_grab", "number", []),
        rcSubdivideAll: wrap("web_rc_subdivide_all", null, []),
        rcSubdivideSelected: wrap("web_rc_subdivide_selected", null, []),
        rcDeleteSelection: wrap("web_rc_delete_selection", null, []),
        rcDeleteStray: wrap("web_rc_delete_stray", null, []),
        rcDeselectAll: wrap("web_rc_deselect_all", null, []),
        rcInvertSelection: wrap("web_rc_invert_selection", null, []),
        rcSelectClusters: wrap("web_rc_select_clusters", null, []),
        rcCenterCamera: wrap("web_rc_center_camera", null, []),
        rcPinSelected: wrap("web_rc_pin_selected", null, []),
        rcUnpinSelected: wrap("web_rc_unpin_selected", null, []),
        setDrawQuadtree: wrap("web_set_draw_quadtree", null, ["number"]),
        getDrawQuadtree: wrap("web_get_draw_quadtree", "number", []),
        setDrawZCurves: wrap("web_set_draw_z_curves", null, ["number"]),
        getDrawZCurves: wrap("web_get_draw_z_curves", "number", [])
      };

      setApi(apiObj);
    };
  }, []);

  return api;
}

function Panel({ title, children, style, onHover, contentStyle = {} }) {
  return html`
    <div
      onMouseEnter=${() => onHover(true)}
      onMouseLeave=${() => onHover(false)}
      style=${{
        pointerEvents: "auto",
        background: "rgba(12, 15, 22, 0.9)",
        border: "1px solid rgba(255,255,255,0.08)",
        borderRadius: 12,
        padding: 16,
        color: "#e6edf3",
        boxShadow: "0 20px 50px rgba(0,0,0,0.45)",
        ...style
      }}
    >
      <div style=${{ fontSize: 14, letterSpacing: "0.16em", color: "#9fb2ff" }}>${title}</div>
      <div style=${{ marginTop: 12, ...contentStyle }}>${children}</div>
    </div>
  `;
}

function Toggle({ label, value, onChange }) {
  return html`
    <label style=${{ display: "flex", alignItems: "center", gap: 10, marginBottom: 10 }}>
      <input type="checkbox" checked=${value} onChange=${(e) => onChange(e.target.checked)} />
      <span>${label}</span>
    </label>
  `;
}

function SectionTitle({ children }) {
  return html`
    <div style=${{ margin: "12px 0 6px", color: "#9aa4b2", textTransform: "uppercase", fontSize: 12, letterSpacing: "0.18em" }}>
      ${children}
    </div>
  `;
}

function Button({ label, onClick, disabled }) {
  return html`
    <button
      onClick=${onClick}
      disabled=${disabled}
      style=${{
        width: "100%",
        textAlign: "left",
        padding: "8px 10px",
        marginBottom: 8,
        background: disabled ? "rgba(255,255,255,0.04)" : "rgba(255,255,255,0.08)",
        border: "1px solid rgba(255,255,255,0.08)",
        borderRadius: 8,
        color: disabled ? "#6b7785" : "inherit",
        cursor: disabled ? "not-allowed" : "pointer"
      }}
    >
      ${label}
    </button>
  `;
}

function Note({ children }) {
  return html`<div style=${{ color: "#6b7785", fontSize: 12, lineHeight: 1.4, marginBottom: 10 }}>${children}</div>`;
}

function Slider({ label, value, min, max, step, onChange }) {
  return html`
    <label style=${{ display: "flex", flexDirection: "column", gap: 6, marginBottom: 12 }}>
      <div style=${{ display: "flex", justifyContent: "space-between" }}>
        <span>${label}</span>
        <span style=${{ color: "#9aa4b2" }}>${value.toFixed(2)}</span>
      </div>
      <input
        type="range"
        min=${min}
        max=${max}
        step=${step}
        value=${value}
        onChange=${(e) => onChange(Number(e.target.value))}
      />
    </label>
  `;
}

function IntSlider({ label, value, min, max, step, onChange }) {
  return html`
    <label style=${{ display: "flex", flexDirection: "column", gap: 6, marginBottom: 12 }}>
      <div style=${{ display: "flex", justifyContent: "space-between" }}>
        <span>${label}</span>
        <span style=${{ color: "#9aa4b2" }}>${value}</span>
      </div>
      <input
        type="range"
        min=${min}
        max=${max}
        step=${step}
        value=${value}
        onChange=${(e) => onChange(Number(e.target.value))}
      />
    </label>
  `;
}

function App() {
  const api = useWasmApi();
  const [timePlaying, setTimePlaying] = useState(true);
  const [timeFactor, setTimeFactor] = useState(1.0);
  const [timeStep, setTimeStep] = useState(1.0);
  const [targetFps, setTargetFps] = useState(144);
  const [gravity, setGravity] = useState(1.0);
  const [drawQuadtree, setDrawQuadtree] = useState(false);
  const [drawZCurves, setDrawZCurves] = useState(false);
  const [toolDrawParticles, setToolDrawParticles] = useState(false);
  const [toolBlackHole, setToolBlackHole] = useState(false);
  const [toolBigGalaxy, setToolBigGalaxy] = useState(false);
  const [toolSmallGalaxy, setToolSmallGalaxy] = useState(false);
  const [toolStar, setToolStar] = useState(false);
  const [toolBigBang, setToolBigBang] = useState(false);
  const [toolPointLight, setToolPointLight] = useState(false);
  const [toolAreaLight, setToolAreaLight] = useState(false);
  const [toolConeLight, setToolConeLight] = useState(false);
  const [toolWall, setToolWall] = useState(false);
  const [toolCircle, setToolCircle] = useState(false);
  const [toolDrawShape, setToolDrawShape] = useState(false);
  const [toolLens, setToolLens] = useState(false);
  const [toolMoveOptics, setToolMoveOptics] = useState(false);
  const [toolEraseOptics, setToolEraseOptics] = useState(false);
  const [toolSelectOptics, setToolSelectOptics] = useState(false);
  const [glowEnabled, setGlowEnabled] = useState(false);
  const [trailsLength, setTrailsLength] = useState(48);
  const [trailsThickness, setTrailsThickness] = useState(0.1);
  const [globalTrails, setGlobalTrails] = useState(false);
  const [selectedTrails, setSelectedTrails] = useState(false);
  const [localTrails, setLocalTrails] = useState(false);
  const [whiteTrails, setWhiteTrails] = useState(false);
  const [colorMode, setColorMode] = useState(0);
  const [pauseAfterRecording, setPauseAfterRecording] = useState(false);
  const [cleanSceneAfterRecording, setCleanSceneAfterRecording] = useState(false);
  const [recordingTimeLimit, setRecordingTimeLimit] = useState(0);
  const [isRecording, setIsRecording] = useState(false);
  const mediaRecorderRef = useRef(null);
  const recordingChunksRef = useRef([]);
  const recordingTimerRef = useRef(null);
  const [toolEraser, setToolEraser] = useState(false);
  const [toolRadialForce, setToolRadialForce] = useState(false);
  const [toolSpin, setToolSpin] = useState(false);
  const [toolGrab, setToolGrab] = useState(false);
  const [showControls, setShowControls] = useState(false);
  const [showInfo, setShowInfo] = useState(false);
  const [showTools, setShowTools] = useState(true);
  const [showParameters, setShowParameters] = useState(true);
  const [showStats, setShowStats] = useState(true);
  const [activeParamTab, setActiveParamTab] = useState("Visuals");
  const [actionChoice, setActionChoice] = useState("");
  const hoverCount = useRef(0);

  const controlsList = useMemo(
    () => [
      "----PARTICLES CREATION----",
      "1. Hold MMB: Paint particles",
      "2. Hold 1 and Drag: Create big galaxy",
      "3. Hold 2 and Drag: Create small galaxy",
      "4. Hold 3 and Drag: Create star",
      "5. Press 4: Create Big Bang",
      "6. C: Clear all particles",
      "",
      "----CAMERA AND SELECTION----",
      "1. Move with RMB",
      "2. Zoom with mouse wheel",
      "3. LCTRL + RMB on cluster to follow it",
      "4. LALT + RMB on particle to follow it",
      "5. LCTRL + LMB on cluster to select it",
      "6. LALT + LMB on particle to select it",
      "7. LCTRL + hold and drag MMB to particle box select",
      "8. LALT + hold and drag MMB to particle box deselect",
      "9. Select on empty space to deselect all",
      "10. Hold SHIFT to add to selection",
      "11. I: Invert selection",
      "12. Z: Center camera on selected particles",
      "13. F: Reset camera",
      "14. D: Deselect all particles",
      "",
      "----UTILITY----",
      "1. TAB: Toggle fullscreen",
      "2. T: Toggle global trails",
      "3. LCTRL + T: Toggle local trails",
      "4. U: Toggle UI",
      "5. RMB on slider to set it to default",
      "6. Use Actions dropdown for extra settings",
      "7. LCTRL + Scroll wheel : Brush size",
      "8. B: Brush attract particles",
      "9. N: Brush spin particles",
      "10. M: Brush grab particles",
      "11. Hold CTRL to invert brush effects",
      "12. R: Record",
      "13. S: Take screenshot",
      "14. X + MMB: Eraser",
      "15. H: Copy selected",
      "16. Hold J and drag: Throw copied",
      "17. Arrows: Control selected particles",
      "18. K: Heat brush",
      "19. L: Cool brush",
      "20. P: Constraint Solids"
    ],
    []
  );

  const infoList = useMemo(
    () => [
      "----INFORMATION----",
      "",
      "Galaxy Engine is a personal project done for learning purposes",
      "by Narcis Calin. The project was entirely made with Raylib",
      "and C++ and it uses external libraries, including ImGui and FFmpeg.",
      "Galaxy Engine is Open Source and the code is available to anyone on GitHub.",
      "Below you can find some useful information:",
      "",
      "1. Theta: Controls the quality of the Barnes-Hut simulation",
      "",
      "2. Barnes-Hut: This is the main gravity algorithm.",
      "",
      "3. Dark Matter: Galaxy Engine simulates dark matter with",
      "invisible particles, which are 5 times heavier than visible ones",
      "",
      "4. Multi-Threading: Parallelizes the simulation across multiple",
      "threads. The default is half the max amount of threads your CPU has,",
      "but it is possible to modify this number.",
      "",
      "5. Collisions: Currently, collisions are experimental. They do not",
      "respect conservation of energy when they are enabled with gravity.",
      "They work as intended when gravity is disabled.",
      "",
      "6. Fluids Mode: This enables fluids for planetary simulation. Each",
      "material has different parameters like stiffness, viscosity,",
      "cohesion, and more.",
      "",
      "7. Frames Export Safe Mode: It is enabled by default when export",
      "frames is enabled. It stores your frames directly to disk, avoiding",
      "filling up your memory. Disabling it will make the render process",
      "much faster, but the program might crash once you run out of memory",
      "",
      "You can report any bugs you may find on our Discord Community."
    ],
    []
  );

  const setUiHover = (hovering) => {
    if (!api) return;
    api.setUiHover(hovering ? 1 : 0);
  };

  const updateHover = (hovering) => {
    hoverCount.current += hovering ? 1 : -1;
    if (hoverCount.current < 0) hoverCount.current = 0;
    setUiHover(hoverCount.current > 0);
  };

  const colorModes = useMemo(
    () => [
      "Solid Color",
      "Density Color",
      "Force Color",
      "Velocity Color",
      "Shockwave Color",
      "Turbulence Color",
      "Pressure Color",
      "Temperature Color",
      "Temperature Gas Color",
      "Material Color"
    ],
    []
  );

  const stopRecording = () => {
    if (recordingTimerRef.current) {
      clearTimeout(recordingTimerRef.current);
      recordingTimerRef.current = null;
    }
    if (mediaRecorderRef.current && mediaRecorderRef.current.state !== "inactive") {
      mediaRecorderRef.current.stop();
    }
  };

  const startRecording = () => {
    const canvas = document.getElementById("canvas");
    if (!canvas || !window.MediaRecorder) return;
    if (mediaRecorderRef.current && mediaRecorderRef.current.state !== "inactive") return;

    const stream = canvas.captureStream(60);
    const preferred = [
      "video/webm;codecs=vp9",
      "video/webm;codecs=vp8",
      "video/webm"
    ];
    const mimeType = preferred.find((type) => MediaRecorder.isTypeSupported(type)) || "";
    const recorder = new MediaRecorder(stream, mimeType ? { mimeType } : undefined);

    recordingChunksRef.current = [];
    recorder.ondataavailable = (event) => {
      if (event.data && event.data.size > 0) {
        recordingChunksRef.current.push(event.data);
      }
    };
    recorder.onstop = () => {
      const blob = new Blob(recordingChunksRef.current, { type: mimeType || "video/webm" });
      const url = URL.createObjectURL(blob);
      const anchor = document.createElement("a");
      const timestamp = new Date().toISOString().replace(/[:.]/g, "-");
      anchor.href = url;
      anchor.download = `galaxy-engine-${timestamp}.webm`;
      anchor.click();
      URL.revokeObjectURL(url);
      recordingChunksRef.current = [];
      setIsRecording(false);
      if (pauseAfterRecording) {
        setTimePlaying(false);
        api?.setTimePlaying(0);
      }
      if (cleanSceneAfterRecording) {
        api?.clearScene();
      }
    };

    recorder.start();
    mediaRecorderRef.current = recorder;
    setIsRecording(true);

    if (recordingTimeLimit > 0) {
      recordingTimerRef.current = setTimeout(() => {
        stopRecording();
      }, recordingTimeLimit * 1000);
    }
  };

  const clearToolStates = () => {
    setToolDrawParticles(false);
    setToolBlackHole(false);
    setToolBigGalaxy(false);
    setToolSmallGalaxy(false);
    setToolStar(false);
    setToolBigBang(false);
    setToolEraser(false);
    setToolRadialForce(false);
    setToolSpin(false);
    setToolGrab(false);
    setToolPointLight(false);
    setToolAreaLight(false);
    setToolConeLight(false);
    setToolWall(false);
    setToolCircle(false);
    setToolDrawShape(false);
    setToolLens(false);
    setToolMoveOptics(false);
    setToolEraseOptics(false);
    setToolSelectOptics(false);
  };

  useEffect(() => {
    if (!api) return;
    setTimePlaying(api.getTimePlaying() === 1);
    setTimeFactor(api.getTimeFactor());
    setTimeStep(api.getTimeStepMultiplier());
    setTargetFps(api.getTargetFps());
    setGravity(api.getGravityMultiplier());
    setDrawQuadtree(api.getDrawQuadtree() === 1);
    setDrawZCurves(api.getDrawZCurves() === 1);
    setToolDrawParticles(api.getToolDrawParticles() === 1);
    setToolBlackHole(api.getToolBlackHole() === 1);
    setToolBigGalaxy(api.getToolBigGalaxy() === 1);
    setToolSmallGalaxy(api.getToolSmallGalaxy() === 1);
    setToolStar(api.getToolStar() === 1);
    setToolBigBang(api.getToolBigBang() === 1);
    setToolPointLight(api.getToolPointLight() === 1);
    setToolAreaLight(api.getToolAreaLight() === 1);
    setToolConeLight(api.getToolConeLight() === 1);
    setToolWall(api.getToolWall() === 1);
    setToolCircle(api.getToolCircle() === 1);
    setToolDrawShape(api.getToolDrawShape() === 1);
    setToolLens(api.getToolLens() === 1);
    setToolMoveOptics(api.getToolMoveOptics() === 1);
    setToolEraseOptics(api.getToolEraseOptics() === 1);
    setToolSelectOptics(api.getToolSelectOptics() === 1);
    setGlowEnabled(false);
    api.setGlowEnabled(0);
    setTrailsLength(api.getTrailsLength());
    setTrailsThickness(api.getTrailsThickness());
    setGlobalTrails(api.getGlobalTrails() === 1);
    setSelectedTrails(api.getSelectedTrails() === 1);
    setLocalTrails(api.getLocalTrails() === 1);
    setWhiteTrails(api.getWhiteTrails() === 1);
    setColorMode(api.getColorMode());
    setPauseAfterRecording(api.getPauseAfterRecording() === 1);
    setCleanSceneAfterRecording(api.getCleanSceneAfterRecording() === 1);
    setRecordingTimeLimit(api.getRecordingTimeLimit());
    setToolEraser(api.getToolEraser() === 1);
    setToolRadialForce(api.getToolRadialForce() === 1);
    setToolSpin(api.getToolSpin() === 1);
    setToolGrab(api.getToolGrab() === 1);
    api.setUiHover(0);
    window.webSetWindowSize = (width, height) => {
      api.setWindowSize(width, height);
    };
    if (window.webSyncCanvasSize) {
      window.webSyncCanvasSize();
    }
  }, [api]);

  useEffect(() => {
    const handlePointerMove = (event) => {
      if (!uiRoot || uiRoot.contains(event.target)) return;
      if (hoverCount.current !== 0) {
        hoverCount.current = 0;
        api?.setUiHover(0);
      }
    };
    window.addEventListener("pointermove", handlePointerMove);
    return () => window.removeEventListener("pointermove", handlePointerMove);
  }, [api]);

  useEffect(() => {
    const canvas = document.getElementById("canvas");
    if (!canvas) return;
    const handler = (event) => {
      event.preventDefault();
    };
    canvas.addEventListener("contextmenu", handler);
    return () => canvas.removeEventListener("contextmenu", handler);
  }, []);

  const actionOptions = useMemo(() => [
    { value: "subdivide_all", label: "Subdivide All", onSelect: () => api?.rcSubdivideAll() },
    { value: "subdivide_selected", label: "Subdivide Selected", onSelect: () => api?.rcSubdivideSelected() },
    { value: "invert_selection", label: "Invert Selection", onSelect: () => api?.rcInvertSelection() },
    { value: "deselect_all", label: "Deselect All", onSelect: () => api?.rcDeselectAll() },
    { value: "select_clusters", label: "Select Clusters", onSelect: () => api?.rcSelectClusters() },
    { value: "delete_selection", label: "Delete Selection", onSelect: () => api?.rcDeleteSelection() },
    { value: "delete_stray", label: "Delete Stray Particles", onSelect: () => api?.rcDeleteStray() },
    { value: "pin_selected", label: "Pin Selected", onSelect: () => api?.rcPinSelected() },
    { value: "unpin_selected", label: "Unpin Selected", onSelect: () => api?.rcUnpinSelected() },
    { value: "center_camera", label: "Center Camera", onSelect: () => api?.rcCenterCamera() }
  ], [api]);

  return html`
    <${React.Fragment}>
      <div
        style=${{
          position: "absolute",
          inset: "var(--edge-inset)",
          display: "grid",
          gridTemplateColumns: "var(--side-width) 1fr var(--side-width)",
          gap: "var(--side-gap)",
          pointerEvents: "none"
        }}
      >
        <div
          style=${{
            display: "flex",
            flexDirection: "column",
            gap: 16,
            pointerEvents: "none",
            background: "transparent",
            border: "none",
            padding: 12,
            height: "100%",
            minHeight: 0,
            boxSizing: "border-box"
          }}
        >
          ${showParameters &&
          html`
            <${Panel} title="PARAMETERS" onHover=${updateHover} style=${{ pointerEvents: "auto" }}>
              <div style=${{ display: "flex", flexWrap: "wrap", gap: 6, marginBottom: 12 }}>
                ${["Visuals", "Physics", "Advanced Stats", "Optics", "Sound", "Recording"].map(
                  (tab) => html`
                    <button
                      key=${tab}
                      onClick=${() => setActiveParamTab(tab)}
                      style=${{
                        padding: "6px 10px",
                        borderRadius: 999,
                        border: "1px solid rgba(255,255,255,0.12)",
                        background: activeParamTab === tab ? "rgba(159,178,255,0.25)" : "transparent",
                        color: activeParamTab === tab ? "#e6edf3" : "#9aa4b2",
                        cursor: "pointer"
                      }}
                    >
                      ${tab}
                    </button>
                  `
                )}
              </div>
              ${activeParamTab === "Visuals" &&
              html`
                <${SectionTitle}>Color Mode</${SectionTitle}>
                <label style=${{ display: "flex", flexDirection: "column", gap: 6, marginBottom: 12 }}>
                  <span>Mode</span>
                  <select
                    value=${colorMode}
                    onChange=${(e) => {
                      const next = Number(e.target.value);
                      setColorMode(next);
                      api?.setColorMode(next);
                    }}
                    style=${{
                      background: "rgba(255,255,255,0.06)",
                      border: "1px solid rgba(255,255,255,0.12)",
                      borderRadius: 8,
                      padding: "6px 8px",
                      color: "#e6edf3"
                    }}
                  >
                    ${colorModes.map(
                      (mode, idx) => html`<option key=${mode} value=${idx}>${mode}</option>`
                    )}
                  </select>
                </label>
                <${SectionTitle}>Render</${SectionTitle}>
                <${Toggle}
                  label="Glow"
                  value=${glowEnabled}
                  onChange=${(val) => {
                    setGlowEnabled(val);
                    api?.setGlowEnabled(val ? 1 : 0);
                  }}
                />
                <${SectionTitle}>Trails</${SectionTitle}>
                <${Toggle}
                  label="Global Trails"
                  value=${globalTrails}
                  onChange=${(val) => {
                    setGlobalTrails(val);
                    if (val) {
                      setSelectedTrails(false);
                      api?.setSelectedTrails(0);
                    }
                    api?.setGlobalTrails(val ? 1 : 0);
                  }}
                />
                <${Toggle}
                  label="Selected Trails"
                  value=${selectedTrails}
                  onChange=${(val) => {
                    setSelectedTrails(val);
                    if (val) {
                      setGlobalTrails(false);
                      api?.setGlobalTrails(0);
                    }
                    api?.setSelectedTrails(val ? 1 : 0);
                  }}
                />
                <${Toggle}
                  label="Local Trails"
                  value=${localTrails}
                  onChange=${(val) => {
                    setLocalTrails(val);
                    api?.setLocalTrails(val ? 1 : 0);
                  }}
                />
                <${Toggle}
                  label="White Trails"
                  value=${whiteTrails}
                  onChange=${(val) => {
                    setWhiteTrails(val);
                    api?.setWhiteTrails(val ? 1 : 0);
                  }}
                />
                <${IntSlider}
                  label="Trails Length"
                  value=${trailsLength}
                  min=${0}
                  max=${1500}
                  step=${1}
                  onChange=${(val) => {
                    setTrailsLength(val);
                    api?.setTrailsLength(val);
                  }}
                />
                <${Slider}
                  label="Trails Thickness"
                  value=${trailsThickness}
                  min=${0.01}
                  max=${1.5}
                  step=${0.01}
                  onChange=${(val) => {
                    setTrailsThickness(val);
                    api?.setTrailsThickness(val);
                  }}
                />
              `}
              ${activeParamTab === "Recording" &&
              html`
                <${SectionTitle}>Recording</${SectionTitle}>
                <${Toggle}
                  label="Pause After Recording"
                  value=${pauseAfterRecording}
                  onChange=${(val) => {
                    setPauseAfterRecording(val);
                    api?.setPauseAfterRecording(val ? 1 : 0);
                  }}
                />
                <${Toggle}
                  label="Clean Scene After Recording"
                  value=${cleanSceneAfterRecording}
                  onChange=${(val) => {
                    setCleanSceneAfterRecording(val);
                    api?.setCleanSceneAfterRecording(val ? 1 : 0);
                  }}
                />
                <${Slider}
                  label="Recording Time Limit (s)"
                  value=${recordingTimeLimit}
                  min=${0}
                  max=${60}
                  step=${0.5}
                  onChange=${(val) => {
                    if (isRecording) return;
                    setRecordingTimeLimit(val);
                    api?.setRecordingTimeLimit(val);
                  }}
                />
                <${Button}
                  label=${isRecording ? "Recording..." : "Start Recording (WebM @ 60fps)"}
                  onClick=${() => {
                    if (!isRecording) startRecording();
                  }}
                  disabled=${isRecording}
                />
                <${Button}
                  label="Stop Recording"
                  onClick=${() => {
                    if (isRecording) stopRecording();
                  }}
                  disabled=${!isRecording}
                />
                <${Note}>Recording downloads a WebM file at 60fps.</${Note}>
              `}
              ${activeParamTab !== "Visuals" && activeParamTab !== "Recording" &&
              html`<${Note}>${activeParamTab} parameters are not wired in the web build yet.</${Note}>`}
            </${Panel}>
          `}
          ${showTools &&
          html`
            <${Panel}
              title="TOOLS"
              onHover=${updateHover}
              style=${{
                pointerEvents: "auto",
                display: "flex",
                flexDirection: "column",
                minHeight: 0,
                flex: "1 1 auto"
              }}
              contentStyle=${{
                overflow: "auto",
                minHeight: 0,
                paddingRight: 6
              }}
            >
              <${SectionTitle}>Particle</${SectionTitle}>
              <${Toggle}
                label="Draw Particles"
                value=${toolDrawParticles}
                onChange=${(val) => {
                  setToolDrawParticles(val);
                  if (val) {
                    clearToolStates();
                    setToolDrawParticles(true);
                    api?.setToolDrawParticles(1);
                  } else {
                    api?.setToolDrawParticles(0);
                  }
                }}
              />
              <${Toggle}
                label="Black Hole"
                value=${toolBlackHole}
                onChange=${(val) => {
                  setToolBlackHole(val);
                  if (val) {
                    clearToolStates();
                    setToolBlackHole(true);
                    api?.setToolBlackHole(1);
                  } else {
                    api?.setToolBlackHole(0);
                  }
                }}
              />
              <${Toggle}
                label="Big Galaxy"
                value=${toolBigGalaxy}
                onChange=${(val) => {
                  setToolBigGalaxy(val);
                  if (val) {
                    clearToolStates();
                    setToolBigGalaxy(true);
                    api?.setToolBigGalaxy(1);
                  } else {
                    api?.setToolBigGalaxy(0);
                  }
                }}
              />
              <${Toggle}
                label="Small Galaxy"
                value=${toolSmallGalaxy}
                onChange=${(val) => {
                  setToolSmallGalaxy(val);
                  if (val) {
                    clearToolStates();
                    setToolSmallGalaxy(true);
                    api?.setToolSmallGalaxy(1);
                  } else {
                    api?.setToolSmallGalaxy(0);
                  }
                }}
              />
              <${Toggle}
                label="Star"
                value=${toolStar}
                onChange=${(val) => {
                  setToolStar(val);
                  if (val) {
                    clearToolStates();
                    setToolStar(true);
                    api?.setToolStar(1);
                  } else {
                    api?.setToolStar(0);
                  }
                }}
              />
              <${Toggle}
                label="Big Bang"
                value=${toolBigBang}
                onChange=${(val) => {
                  setToolBigBang(val);
                  if (val) {
                    clearToolStates();
                    setToolBigBang(true);
                    api?.setToolBigBang(1);
                  } else {
                    api?.setToolBigBang(0);
                  }
                }}
              />
              <${SectionTitle}>Brush</${SectionTitle}>
              <${Toggle}
                label="Eraser"
                value=${toolEraser}
                onChange=${(val) => {
                  setToolEraser(val);
                  if (val) {
                    clearToolStates();
                    setToolEraser(true);
                    api?.setToolEraser(1);
                  } else {
                    api?.setToolEraser(0);
                  }
                }}
              />
              <${Toggle}
                label="Radial Force"
                value=${toolRadialForce}
                onChange=${(val) => {
                  setToolRadialForce(val);
                  if (val) {
                    clearToolStates();
                    setToolRadialForce(true);
                    api?.setToolRadialForce(1);
                  } else {
                    api?.setToolRadialForce(0);
                  }
                }}
              />
              <${Toggle}
                label="Spin"
                value=${toolSpin}
                onChange=${(val) => {
                  setToolSpin(val);
                  if (val) {
                    clearToolStates();
                    setToolSpin(true);
                    api?.setToolSpin(1);
                  } else {
                    api?.setToolSpin(0);
                  }
                }}
              />
              <${Toggle}
                label="Grab"
                value=${toolGrab}
                onChange=${(val) => {
                  setToolGrab(val);
                  if (val) {
                    clearToolStates();
                    setToolGrab(true);
                    api?.setToolGrab(1);
                  } else {
                    api?.setToolGrab(0);
                  }
                }}
              />
              <${SectionTitle}>Optics</${SectionTitle}>
              <${Toggle}
                label="Point Light"
                value=${toolPointLight}
                onChange=${(val) => {
                  setToolPointLight(val);
                  if (val) {
                    clearToolStates();
                    setToolPointLight(true);
                    api?.setToolPointLight(1);
                  } else {
                    api?.setToolPointLight(0);
                  }
                }}
              />
              <${Toggle}
                label="Area Light"
                value=${toolAreaLight}
                onChange=${(val) => {
                  setToolAreaLight(val);
                  if (val) {
                    clearToolStates();
                    setToolAreaLight(true);
                    api?.setToolAreaLight(1);
                  } else {
                    api?.setToolAreaLight(0);
                  }
                }}
              />
              <${Toggle}
                label="Cone Light"
                value=${toolConeLight}
                onChange=${(val) => {
                  setToolConeLight(val);
                  if (val) {
                    clearToolStates();
                    setToolConeLight(true);
                    api?.setToolConeLight(1);
                  } else {
                    api?.setToolConeLight(0);
                  }
                }}
              />
              <${Toggle}
                label="Wall"
                value=${toolWall}
                onChange=${(val) => {
                  setToolWall(val);
                  if (val) {
                    clearToolStates();
                    setToolWall(true);
                    api?.setToolWall(1);
                  } else {
                    api?.setToolWall(0);
                  }
                }}
              />
              <${Toggle}
                label="Circle"
                value=${toolCircle}
                onChange=${(val) => {
                  setToolCircle(val);
                  if (val) {
                    clearToolStates();
                    setToolCircle(true);
                    api?.setToolCircle(1);
                  } else {
                    api?.setToolCircle(0);
                  }
                }}
              />
              <${Toggle}
                label="Draw Shape"
                value=${toolDrawShape}
                onChange=${(val) => {
                  setToolDrawShape(val);
                  if (val) {
                    clearToolStates();
                    setToolDrawShape(true);
                    api?.setToolDrawShape(1);
                  } else {
                    api?.setToolDrawShape(0);
                  }
                }}
              />
              <${Toggle}
                label="Lens"
                value=${toolLens}
                onChange=${(val) => {
                  setToolLens(val);
                  if (val) {
                    clearToolStates();
                    setToolLens(true);
                    api?.setToolLens(1);
                  } else {
                    api?.setToolLens(0);
                  }
                }}
              />
              <${Toggle}
                label="Move Optics"
                value=${toolMoveOptics}
                onChange=${(val) => {
                  setToolMoveOptics(val);
                  if (val) {
                    clearToolStates();
                    setToolMoveOptics(true);
                    api?.setToolMoveOptics(1);
                  } else {
                    api?.setToolMoveOptics(0);
                  }
                }}
              />
              <${Toggle}
                label="Erase Optics"
                value=${toolEraseOptics}
                onChange=${(val) => {
                  setToolEraseOptics(val);
                  if (val) {
                    clearToolStates();
                    setToolEraseOptics(true);
                    api?.setToolEraseOptics(1);
                  } else {
                    api?.setToolEraseOptics(0);
                  }
                }}
              />
              <${Toggle}
                label="Select Optics"
                value=${toolSelectOptics}
                onChange=${(val) => {
                  setToolSelectOptics(val);
                  if (val) {
                    clearToolStates();
                    setToolSelectOptics(true);
                    api?.setToolSelectOptics(1);
                  } else {
                    api?.setToolSelectOptics(0);
                  }
                }}
              />
            </${Panel}>
          `}
        </div>

        <div />

        <div
          style=${{
            display: "flex",
            flexDirection: "column",
            gap: 16,
            pointerEvents: "none",
            background: "transparent",
            border: "none",
            padding: 12,
            height: "100%",
            boxSizing: "border-box"
          }}
        >
          <${Panel} title="ACTIONS" onHover=${updateHover} style=${{ pointerEvents: "auto" }}>
            <label style=${{ display: "flex", flexDirection: "column", gap: 6 }}>
              <span>Quick Action</span>
              <select
                value=${actionChoice}
                onChange=${(e) => {
                  const next = e.target.value;
                  if (!next) return;
                  const action = actionOptions.find((option) => option.value === next);
                  action?.onSelect();
                  setActionChoice("");
                }}
                style=${{
                  background: "rgba(255,255,255,0.06)",
                  border: "1px solid rgba(255,255,255,0.12)",
                  borderRadius: 8,
                  padding: "6px 8px",
                  color: "#e6edf3"
                }}
              >
                <option value="">Select an action…</option>
                ${actionOptions.map(
                  (option) => html`<option key=${option.value} value=${option.value}>${option.label}</option>`
                )}
              </select>
            </label>
            <${Note}>Runs immediately and closes after selection.</${Note}>
          </${Panel}>
          <${Panel} title="SETTINGS" onHover=${updateHover} style=${{ pointerEvents: "auto", maxHeight: "60vh", overflow: "auto" }}>
            <${SectionTitle}>Panels</${SectionTitle}>
            <${Toggle} label="Show Controls" value=${showControls} onChange=${setShowControls} />
            <${Toggle} label="Show Information" value=${showInfo} onChange=${setShowInfo} />
            <${Toggle} label="Show Tools" value=${showTools} onChange=${setShowTools} />
            <${Toggle} label="Show Parameters" value=${showParameters} onChange=${setShowParameters} />
            <${Toggle} label="Show Stats" value=${showStats} onChange=${setShowStats} />

            <${SectionTitle}>Simulation</${SectionTitle}>
            <${Button}
              label=${timePlaying ? "Pause" : "Continue"}
              onClick=${() => {
                const next = !timePlaying;
                setTimePlaying(next);
                api?.setTimePlaying(next ? 1 : 0);
              }}
            />
            <${Toggle}
              label="Play / Pause"
              value=${timePlaying}
              onChange=${(val) => {
                setTimePlaying(val);
                api?.setTimePlaying(val ? 1 : 0);
              }}
            />
            <${Slider}
              label="Time Factor"
              value=${timeFactor}
              min=${0}
              max=${5}
              step=${0.05}
              onChange=${(val) => {
                setTimeFactor(val);
                api?.setTimeFactor(val);
              }}
            />
            <${Slider}
              label="Time Step"
              value=${timeStep}
              min=${0.1}
              max=${5}
              step=${0.05}
              onChange=${(val) => {
                setTimeStep(val);
                api?.setTimeStepMultiplier(val);
              }}
            />
            <${SectionTitle}>Physics</${SectionTitle}>
            <${Slider}
              label="Gravity Multiplier"
              value=${gravity}
              min=${0}
              max=${5}
              step=${0.05}
              onChange=${(val) => {
                setGravity(val);
                api?.setGravityMultiplier(val);
              }}
            />
            <${SectionTitle}>Performance</${SectionTitle}>
            <label style=${{ display: "flex", flexDirection: "column", gap: 6, marginBottom: 12 }}>
              <span>Target FPS</span>
              <input
                type="number"
                min=${1}
                max=${240}
                value=${targetFps}
                onChange=${(e) => {
                  const next = Number(e.target.value || 0);
                  setTargetFps(next);
                  api?.setTargetFps(next);
                }}
              />
            </label>
            <${SectionTitle}>Debug</${SectionTitle}>
            <${Toggle}
              label="Draw Quadtree"
              value=${drawQuadtree}
              onChange=${(val) => {
                setDrawQuadtree(val);
                api?.setDrawQuadtree(val ? 1 : 0);
              }}
            />
            <${Toggle}
              label="Draw Z-Curves"
              value=${drawZCurves}
              onChange=${(val) => {
                setDrawZCurves(val);
                api?.setDrawZCurves(val ? 1 : 0);
              }}
            />
            <${Note}>More settings are available in the native UI and will be wired here next.</${Note}>
          </${Panel}>

          ${showStats &&
          html`
            <${Panel} title="STATS" onHover=${updateHover} style=${{ pointerEvents: "auto" }}>
              <div style=${{ display: "flex", justifyContent: "space-between", marginBottom: 6 }}>
                <span>Target FPS</span>
                <span>${targetFps}</span>
              </div>
              <div style=${{ display: "flex", justifyContent: "space-between", marginBottom: 6 }}>
                <span>Time Factor</span>
                <span>${timeFactor.toFixed(2)}</span>
              </div>
              <div style=${{ display: "flex", justifyContent: "space-between", marginBottom: 6 }}>
                <span>Gravity Multiplier</span>
                <span>${gravity.toFixed(2)}</span>
              </div>
              <${Note}>Native stats (FPS, particles, lights) are not exposed yet.</${Note}>
            </${Panel}>
          `}
        </div>
      </div>

      ${showControls &&
      html`
        <${Panel}
          title="CONTROLS"
          onHover=${updateHover}
          style=${{ position: "absolute", top: 120, left: "50%", width: 420, transform: "translateX(-50%)", zIndex: 3 }}
        >
          ${controlsList.map(
            (line, idx) =>
              html`<div key=${idx} style=${{ color: line.startsWith("----") ? "#9fb2ff" : "#e6edf3", marginBottom: 4 }}>
                ${line}
              </div>`
          )}
        </${Panel}>
      `}

      ${showInfo &&
      html`
        <${Panel}
          title="INFORMATION"
          onHover=${updateHover}
          style=${{ position: "absolute", top: 120, left: "50%", width: 520, transform: "translateX(-50%)", zIndex: 3 }}
        >
          ${infoList.map(
            (line, idx) =>
              html`<div key=${idx} style=${{ color: line.startsWith("----") ? "#9fb2ff" : "#e6edf3", marginBottom: 4 }}>
                ${line}
              </div>`
          )}
          <div style=${{ display: "flex", gap: 8, marginTop: 12, flexWrap: "wrap" }}>
            <a href="https://github.com/NarcisCalin/Galaxy-Engine" target="_blank" rel="noreferrer">GitHub</a>
            <a href="https://discord.gg/Xd5JUqNFPM" target="_blank" rel="noreferrer">Discord</a>
            <a href="https://open.spotify.com/artist/1vrrvzYvRY27SDZp7WsMwx" target="_blank" rel="noreferrer">Soundtrack</a>
          </div>
          <${Note}>Links open in a new tab.</${Note}>
        </${Panel}>
      `}

    </${React.Fragment}>
  `;
}

root.render(html`<${App} />`);
