import React, { useEffect, useMemo, useRef, useState } from "https://esm.sh/react@18";
import { createRoot } from "https://esm.sh/react-dom@18/client";
import htm from "https://esm.sh/htm@3";

const html = htm.bind(React.createElement);

const uiRoot = document.getElementById("ui-root");
const root = createRoot(uiRoot);
const STORAGE_KEY = "galaxy-engine-ui-state-v1";
const LEFT_TABS = ["Visuals", "Physics", "Advanced Stats", "Optics", "Sound", "Recording"];

function loadUiState() {
  try {
    const raw = localStorage.getItem(STORAGE_KEY);
    if (!raw) return null;
    const parsed = JSON.parse(raw);
    if (!parsed || typeof parsed !== "object") return null;
    return parsed;
  } catch (err) {
    return null;
  }
}

function saveUiState(state) {
  try {
    localStorage.setItem(STORAGE_KEY, JSON.stringify(state));
  } catch (err) {
    // Ignore storage failures (private mode, quota, etc.)
  }
}

function clampNumber(value, min, max) {
  return Math.min(max, Math.max(min, value));
}

function normalizeColor(color) {
  if (!color || typeof color !== "object") {
    return { r: 255, g: 255, b: 255, a: 255 };
  }
  return {
    r: clampNumber(Math.round(color.r ?? 0), 0, 255),
    g: clampNumber(Math.round(color.g ?? 0), 0, 255),
    b: clampNumber(Math.round(color.b ?? 0), 0, 255),
    a: clampNumber(Math.round(color.a ?? 255), 0, 255)
  };
}

function packColor(color) {
  const normalized = normalizeColor(color);
  return (
    (normalized.r & 255) |
    ((normalized.g & 255) << 8) |
    ((normalized.b & 255) << 16) |
    ((normalized.a & 255) << 24)
  ) >>> 0;
}

function unpackColor(value) {
  const packed = (value ?? 0) >>> 0;
  return {
    r: packed & 255,
    g: (packed >>> 8) & 255,
    b: (packed >>> 16) & 255,
    a: (packed >>> 24) & 255
  };
}

function rgbToHex(color) {
  const normalized = normalizeColor(color);
  return (
    "#" +
    [normalized.r, normalized.g, normalized.b]
      .map((channel) => channel.toString(16).padStart(2, "0"))
      .join("")
  );
}

function hexToRgb(hex) {
  if (typeof hex !== "string") {
    return { r: 255, g: 255, b: 255 };
  }
  const sanitized = hex.replace("#", "");
  if (sanitized.length !== 6) {
    return { r: 255, g: 255, b: 255 };
  }
  const value = Number.parseInt(sanitized, 16);
  return {
    r: (value >> 16) & 255,
    g: (value >> 8) & 255,
    b: value & 255
  };
}

const TAR_BLOCK_SIZE = 512;

function writeTarString(buffer, offset, value, length) {
  const str = value || "";
  for (let i = 0; i < length; i += 1) {
    buffer[offset + i] = i < str.length ? str.charCodeAt(i) : 0;
  }
}

function writeTarOctal(buffer, offset, value, length) {
  const str = value.toString(8).padStart(length - 1, "0") + "\0";
  writeTarString(buffer, offset, str, length);
}

function createTarHeader(name, size, mtime) {
  const buffer = new Uint8Array(TAR_BLOCK_SIZE);
  writeTarString(buffer, 0, name, 100);
  writeTarOctal(buffer, 100, 0o644, 8);
  writeTarOctal(buffer, 108, 0, 8);
  writeTarOctal(buffer, 116, 0, 8);
  writeTarOctal(buffer, 124, size, 12);
  writeTarOctal(buffer, 136, mtime, 12);
  for (let i = 148; i < 156; i += 1) {
    buffer[i] = 32;
  }
  buffer[156] = "0".charCodeAt(0);
  writeTarString(buffer, 257, "ustar", 5);
  writeTarString(buffer, 263, "00", 2);
  let checksum = 0;
  for (let i = 0; i < TAR_BLOCK_SIZE; i += 1) {
    checksum += buffer[i];
  }
  const checksumStr = checksum.toString(8).padStart(6, "0") + "\0 ";
  writeTarString(buffer, 148, checksumStr, 8);
  return buffer;
}

async function buildTar(files) {
  const parts = [];
  const mtime = Math.floor(Date.now() / 1000);
  for (const file of files) {
    const data =
      file.data instanceof Uint8Array ? file.data : new Uint8Array(await file.blob.arrayBuffer());
    parts.push(createTarHeader(file.name, data.length, mtime));
    parts.push(data);
    const padding = (TAR_BLOCK_SIZE - (data.length % TAR_BLOCK_SIZE)) % TAR_BLOCK_SIZE;
    if (padding) {
      parts.push(new Uint8Array(padding));
    }
  }
  parts.push(new Uint8Array(TAR_BLOCK_SIZE * 2));
  return new Blob(parts, { type: "application/x-tar" });
}

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
        setThreadsAmount: wrap("web_set_threads_amount", null, ["number"]),
        getThreadsAmount: wrap("web_get_threads_amount", "number", []),
        setDomainWidth: wrap("web_set_domain_width", null, ["number"]),
        getDomainWidth: wrap("web_get_domain_width", "number", []),
        setDomainHeight: wrap("web_set_domain_height", null, ["number"]),
        getDomainHeight: wrap("web_get_domain_height", "number", []),
        setBlackHoleInitMass: wrap("web_set_black_hole_init_mass", null, ["number"]),
        getBlackHoleInitMass: wrap("web_get_black_hole_init_mass", "number", []),
        setAmbientTemperature: wrap("web_set_ambient_temperature", null, ["number"]),
        getAmbientTemperature: wrap("web_get_ambient_temperature", "number", []),
        setAmbientHeatRate: wrap("web_set_ambient_heat_rate", null, ["number"]),
        getAmbientHeatRate: wrap("web_get_ambient_heat_rate", "number", []),
        setHeatConductivity: wrap("web_set_heat_conductivity", null, ["number"]),
        getHeatConductivity: wrap("web_get_heat_conductivity", "number", []),
        setConstraintStiffness: wrap("web_set_constraint_stiffness", null, ["number"]),
        getConstraintStiffness: wrap("web_get_constraint_stiffness", "number", []),
        setConstraintResistance: wrap("web_set_constraint_resistance", null, ["number"]),
        getConstraintResistance: wrap("web_get_constraint_resistance", "number", []),
        setFluidVerticalGravity: wrap("web_set_fluid_vertical_gravity", null, ["number"]),
        getFluidVerticalGravity: wrap("web_get_fluid_vertical_gravity", "number", []),
        setFluidMassMultiplier: wrap("web_set_fluid_mass_multiplier", null, ["number"]),
        getFluidMassMultiplier: wrap("web_get_fluid_mass_multiplier", "number", []),
        setFluidViscosity: wrap("web_set_fluid_viscosity", null, ["number"]),
        getFluidViscosity: wrap("web_get_fluid_viscosity", "number", []),
        setFluidStiffness: wrap("web_set_fluid_stiffness", null, ["number"]),
        getFluidStiffness: wrap("web_get_fluid_stiffness", "number", []),
        setFluidCohesion: wrap("web_set_fluid_cohesion", null, ["number"]),
        getFluidCohesion: wrap("web_get_fluid_cohesion", "number", []),
        setFluidDelta: wrap("web_set_fluid_delta", null, ["number"]),
        getFluidDelta: wrap("web_get_fluid_delta", "number", []),
        setFluidMaxVelocity: wrap("web_set_fluid_max_velocity", null, ["number"]),
        getFluidMaxVelocity: wrap("web_get_fluid_max_velocity", "number", []),
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
        setParticleSizeMultiplier: wrap("web_set_particle_size_multiplier", null, ["number"]),
        getParticleSizeMultiplier: wrap("web_get_particle_size_multiplier", "number", []),
        setTheta: wrap("web_set_theta", null, ["number"]),
        getTheta: wrap("web_get_theta", "number", []),
        setSoftening: wrap("web_set_softening", null, ["number"]),
        getSoftening: wrap("web_get_softening", "number", []),
        setOpticsEnabled: wrap("web_set_optics_enabled", null, ["number"]),
        getOpticsEnabled: wrap("web_get_optics_enabled", "number", []),
        setLightGain: wrap("web_set_light_gain", null, ["number"]),
        getLightGain: wrap("web_get_light_gain", "number", []),
        setLightSpread: wrap("web_set_light_spread", null, ["number"]),
        getLightSpread: wrap("web_get_light_spread", "number", []),
        setWallSpecularRoughness: wrap("web_set_wall_specular_roughness", null, ["number"]),
        getWallSpecularRoughness: wrap("web_get_wall_specular_roughness", "number", []),
        setWallRefractionRoughness: wrap("web_set_wall_refraction_roughness", null, ["number"]),
        getWallRefractionRoughness: wrap("web_get_wall_refraction_roughness", "number", []),
        setWallRefractionAmount: wrap("web_set_wall_refraction_amount", null, ["number"]),
        getWallRefractionAmount: wrap("web_get_wall_refraction_amount", "number", []),
        setWallIor: wrap("web_set_wall_ior", null, ["number"]),
        getWallIor: wrap("web_get_wall_ior", "number", []),
        setWallDispersion: wrap("web_set_wall_dispersion", null, ["number"]),
        getWallDispersion: wrap("web_get_wall_dispersion", "number", []),
        setWallEmissionGain: wrap("web_set_wall_emission_gain", null, ["number"]),
        getWallEmissionGain: wrap("web_get_wall_emission_gain", "number", []),
        setShapeRelaxIter: wrap("web_set_shape_relax_iter", null, ["number"]),
        getShapeRelaxIter: wrap("web_get_shape_relax_iter", "number", []),
        setShapeRelaxFactor: wrap("web_set_shape_relax_factor", null, ["number"]),
        getShapeRelaxFactor: wrap("web_get_shape_relax_factor", "number", []),
        setMaxSamples: wrap("web_set_max_samples", null, ["number"]),
        getMaxSamples: wrap("web_get_max_samples", "number", []),
        setSampleRaysAmount: wrap("web_set_sample_rays_amount", null, ["number"]),
        getSampleRaysAmount: wrap("web_get_sample_rays_amount", "number", []),
        setMaxBounces: wrap("web_set_max_bounces", null, ["number"]),
        getMaxBounces: wrap("web_get_max_bounces", "number", []),
        setDiffuseEnabled: wrap("web_set_diffuse_enabled", null, ["number"]),
        getDiffuseEnabled: wrap("web_get_diffuse_enabled", "number", []),
        setSpecularEnabled: wrap("web_set_specular_enabled", null, ["number"]),
        getSpecularEnabled: wrap("web_get_specular_enabled", "number", []),
        setRefractionEnabled: wrap("web_set_refraction_enabled", null, ["number"]),
        getRefractionEnabled: wrap("web_get_refraction_enabled", "number", []),
        setDispersionEnabled: wrap("web_set_dispersion_enabled", null, ["number"]),
        getDispersionEnabled: wrap("web_get_dispersion_enabled", "number", []),
        setEmissionEnabled: wrap("web_set_emission_enabled", null, ["number"]),
        getEmissionEnabled: wrap("web_get_emission_enabled", "number", []),
        setSymmetricalLens: wrap("web_set_symmetrical_lens", null, ["number"]),
        getSymmetricalLens: wrap("web_get_symmetrical_lens", "number", []),
        setDrawNormals: wrap("web_set_draw_normals", null, ["number"]),
        getDrawNormals: wrap("web_get_draw_normals", "number", []),
        setRelaxMove: wrap("web_set_relax_move", null, ["number"]),
        getRelaxMove: wrap("web_get_relax_move", "number", []),
        setLightColor: wrap("web_set_light_color", null, ["number"]),
        getLightColor: wrap("web_get_light_color", "number", []),
        setWallBaseColor: wrap("web_set_wall_base_color", null, ["number"]),
        getWallBaseColor: wrap("web_get_wall_base_color", "number", []),
        setWallSpecularColor: wrap("web_set_wall_specular_color", null, ["number"]),
        getWallSpecularColor: wrap("web_get_wall_specular_color", "number", []),
        setWallRefractionColor: wrap("web_set_wall_refraction_color", null, ["number"]),
        getWallRefractionColor: wrap("web_get_wall_refraction_color", "number", []),
        setWallEmissionColor: wrap("web_set_wall_emission_color", null, ["number"]),
        getWallEmissionColor: wrap("web_get_wall_emission_color", "number", []),
        getLightingSamples: wrap("web_get_lighting_samples", "number", []),
        resetLightingSamples: wrap("web_reset_lighting_samples", null, []),
        setWindowSize: wrap("web_set_window_size", null, ["number", "number"]),
        setPauseAfterRecording: wrap("web_set_pause_after_recording", null, ["number"]),
        getPauseAfterRecording: wrap("web_get_pause_after_recording", "number", []),
        setCleanSceneAfterRecording: wrap("web_set_clean_scene_after_recording", null, ["number"]),
        getCleanSceneAfterRecording: wrap("web_get_clean_scene_after_recording", "number", []),
        setRecordingTimeLimit: wrap("web_set_recording_time_limit", null, ["number"]),
        getRecordingTimeLimit: wrap("web_get_recording_time_limit", "number", []),
        setShowBrushCursor: wrap("web_set_show_brush_cursor", null, ["number"]),
        getShowBrushCursor: wrap("web_get_show_brush_cursor", "number", []),
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

function Panel({ title, children, style, onHover, contentStyle = {}, collapsed = false, onToggle }) {
  const canToggle = typeof onToggle === "function";
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
      <div style=${{ display: "flex", alignItems: "center", justifyContent: "space-between", gap: 12 }}>
        <div style=${{ fontSize: 14, letterSpacing: "0.16em", color: "#9fb2ff" }}>${title}</div>
        ${canToggle &&
        html`
          <button
            onClick=${onToggle}
            style=${{
              background: "rgba(255,255,255,0.06)",
              border: "1px solid rgba(255,255,255,0.12)",
              borderRadius: 6,
              padding: "2px 8px",
              color: "#c7d1dd",
              cursor: "pointer"
            }}
          >
            ${collapsed ? "+" : "-"}
          </button>
        `}
      </div>
      ${!collapsed && html`<div style=${{ marginTop: 12, ...contentStyle }}>${children}</div>`}
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

function Slider({ label, value, min, max, step, onChange, tooltip }) {
  return html`
    <label
      style=${{ display: "flex", flexDirection: "column", gap: 6, marginBottom: 12 }}
      title=${tooltip || ""}
    >
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
        title=${tooltip || ""}
      />
    </label>
  `;
}

function IntSlider({ label, value, min, max, step, onChange, tooltip }) {
  return html`
    <label
      style=${{ display: "flex", flexDirection: "column", gap: 6, marginBottom: 12 }}
      title=${tooltip || ""}
    >
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
        title=${tooltip || ""}
      />
    </label>
  `;
}

function LogIntSlider({ label, value, max, onChange, tooltip }) {
  const logMax = Math.log10(max + 1);
  const sliderValue = Math.round((Math.log10(value + 1) / logMax) * 100);
  return html`
    <label
      style=${{ display: "flex", flexDirection: "column", gap: 6, marginBottom: 12 }}
      title=${tooltip || ""}
    >
      <div style=${{ display: "flex", justifyContent: "space-between" }}>
        <span>${label}</span>
        <span style=${{ color: "#9aa4b2" }}>${value}</span>
      </div>
      <input
        type="range"
        min=${0}
        max=${100}
        step=${1}
        value=${sliderValue}
        onChange=${(e) => {
          const raw = Number(e.target.value);
          const next = Math.round(Math.pow(10, (raw / 100) * logMax) - 1);
          onChange(next);
        }}
        title=${tooltip || ""}
      />
    </label>
  `;
}

function ColorPicker({ label, color, onChange }) {
  const normalized = normalizeColor(color);
  const hexValue = rgbToHex(normalized);
  return html`
    <div style=${{ marginBottom: 12 }}>
      <label style=${{ display: "flex", flexDirection: "column", gap: 6, marginBottom: 6 }}>
        <span>${label}</span>
        <div style=${{ display: "flex", alignItems: "center", gap: 10 }}>
          <input
            type="color"
            value=${hexValue}
            onChange=${(e) => {
              const rgb = hexToRgb(e.target.value);
              onChange({ ...normalized, ...rgb });
            }}
          />
          <span style=${{ color: "#9aa4b2", fontSize: 12 }}>${hexValue.toUpperCase()}</span>
        </div>
      </label>
      <${IntSlider}
        label="${label} Alpha"
        value=${normalized.a}
        min=${0}
        max=${255}
        step=${1}
        onChange=${(val) => onChange({ ...normalized, a: val })}
      />
    </div>
  `;
}

function App() {
  const api = useWasmApi();
  const defaultColorMode = 2;
  const defaultTrailsLength = 8;
  const trailsLengthMax = 1500;
  const storedStateRef = useRef(loadUiState());
  const storedState = storedStateRef.current || {};
  const initialActiveTab = LEFT_TABS.includes(storedState.activeParamTab)
    ? storedState.activeParamTab
    : "Visuals";
  const [timePlaying, setTimePlaying] = useState(storedState.timePlaying ?? true);
  const [timeStep, setTimeStep] = useState(storedState.timeStep ?? 1.0);
  const [targetFps, setTargetFps] = useState(storedState.targetFps ?? 144);
  const [gravity, setGravity] = useState(storedState.gravity ?? 1.0);
  const [particleSizeMultiplier, setParticleSizeMultiplier] = useState(storedState.particleSizeMultiplier ?? 1.0);
  const [theta, setTheta] = useState(storedState.theta ?? 0.8);
  const [softening, setSoftening] = useState(storedState.softening ?? 2.5);
  const [glowEnabled, setGlowEnabled] = useState(storedState.glowEnabled ?? false);
  const [threadsAmount, setThreadsAmount] = useState(storedState.threadsAmount ?? 1);
  const [domainWidth, setDomainWidth] = useState(storedState.domainWidth ?? 3840);
  const [domainHeight, setDomainHeight] = useState(storedState.domainHeight ?? 2160);
  const [blackHoleInitMass, setBlackHoleInitMass] = useState(storedState.blackHoleInitMass ?? 1.0);
  const [ambientTemperature, setAmbientTemperature] = useState(storedState.ambientTemperature ?? 274.0);
  const [ambientHeatRate, setAmbientHeatRate] = useState(storedState.ambientHeatRate ?? 1.0);
  const [heatConductivity, setHeatConductivity] = useState(storedState.heatConductivity ?? 0.045);
  const [constraintStiffness, setConstraintStiffness] = useState(storedState.constraintStiffness ?? 1.0);
  const [constraintResistance, setConstraintResistance] = useState(storedState.constraintResistance ?? 1.0);
  const [fluidVerticalGravity, setFluidVerticalGravity] = useState(storedState.fluidVerticalGravity ?? 3.0);
  const [fluidMassMultiplier, setFluidMassMultiplier] = useState(storedState.fluidMassMultiplier ?? 0.03);
  const [fluidViscosity, setFluidViscosity] = useState(storedState.fluidViscosity ?? 0.1);
  const [fluidStiffness, setFluidStiffness] = useState(storedState.fluidStiffness ?? 1.0);
  const [fluidCohesion, setFluidCohesion] = useState(storedState.fluidCohesion ?? 1.0);
  const [fluidDelta, setFluidDelta] = useState(storedState.fluidDelta ?? 9500.0);
  const [fluidMaxVelocity, setFluidMaxVelocity] = useState(storedState.fluidMaxVelocity ?? 250.0);
  const [opticsEnabled, setOpticsEnabled] = useState(storedState.opticsEnabled ?? false);
  const [lightGain, setLightGain] = useState(storedState.lightGain ?? 0.25);
  const [lightSpread, setLightSpread] = useState(storedState.lightSpread ?? 0.85);
  const [wallSpecularRoughness, setWallSpecularRoughness] = useState(storedState.wallSpecularRoughness ?? 0.5);
  const [wallRefractionRoughness, setWallRefractionRoughness] = useState(storedState.wallRefractionRoughness ?? 0.0);
  const [wallRefractionAmount, setWallRefractionAmount] = useState(storedState.wallRefractionAmount ?? 0.0);
  const [wallIor, setWallIor] = useState(storedState.wallIor ?? 1.5);
  const [wallDispersion, setWallDispersion] = useState(storedState.wallDispersion ?? 0.0);
  const [wallEmissionGain, setWallEmissionGain] = useState(storedState.wallEmissionGain ?? 0.0);
  const [shapeRelaxIter, setShapeRelaxIter] = useState(storedState.shapeRelaxIter ?? 15);
  const [shapeRelaxFactor, setShapeRelaxFactor] = useState(storedState.shapeRelaxFactor ?? 0.65);
  const [maxSamples, setMaxSamples] = useState(storedState.maxSamples ?? 500);
  const [sampleRaysAmount, setSampleRaysAmount] = useState(storedState.sampleRaysAmount ?? 1024);
  const [maxBounces, setMaxBounces] = useState(storedState.maxBounces ?? 3);
  const [diffuseEnabled, setDiffuseEnabled] = useState(storedState.diffuseEnabled ?? true);
  const [specularEnabled, setSpecularEnabled] = useState(storedState.specularEnabled ?? true);
  const [refractionEnabled, setRefractionEnabled] = useState(storedState.refractionEnabled ?? true);
  const [dispersionEnabled, setDispersionEnabled] = useState(storedState.dispersionEnabled ?? true);
  const [emissionEnabled, setEmissionEnabled] = useState(storedState.emissionEnabled ?? true);
  const [symmetricalLens, setSymmetricalLens] = useState(storedState.symmetricalLens ?? false);
  const [drawNormals, setDrawNormals] = useState(storedState.drawNormals ?? false);
  const [relaxMove, setRelaxMove] = useState(storedState.relaxMove ?? false);
  const [lightColor, setLightColor] = useState(
    normalizeColor(storedState.lightColor ?? { r: 255, g: 255, b: 255, a: 64 })
  );
  const [wallBaseColor, setWallBaseColor] = useState(
    normalizeColor(storedState.wallBaseColor ?? { r: 200, g: 200, b: 200, a: 255 })
  );
  const [wallSpecularColor, setWallSpecularColor] = useState(
    normalizeColor(storedState.wallSpecularColor ?? { r: 255, g: 255, b: 255, a: 255 })
  );
  const [wallRefractionColor, setWallRefractionColor] = useState(
    normalizeColor(storedState.wallRefractionColor ?? { r: 255, g: 255, b: 255, a: 255 })
  );
  const [wallEmissionColor, setWallEmissionColor] = useState(
    normalizeColor(storedState.wallEmissionColor ?? { r: 255, g: 255, b: 255, a: 0 })
  );
  const [lightingSamples, setLightingSamples] = useState(0);
  const [drawQuadtree, setDrawQuadtree] = useState(storedState.drawQuadtree ?? false);
  const [drawZCurves, setDrawZCurves] = useState(storedState.drawZCurves ?? false);
  const [toolDrawParticles, setToolDrawParticles] = useState(storedState.toolDrawParticles ?? false);
  const [toolBlackHole, setToolBlackHole] = useState(storedState.toolBlackHole ?? false);
  const [toolBigGalaxy, setToolBigGalaxy] = useState(storedState.toolBigGalaxy ?? false);
  const [toolSmallGalaxy, setToolSmallGalaxy] = useState(storedState.toolSmallGalaxy ?? false);
  const [toolStar, setToolStar] = useState(storedState.toolStar ?? false);
  const [toolBigBang, setToolBigBang] = useState(storedState.toolBigBang ?? false);
  const [toolPointLight, setToolPointLight] = useState(storedState.toolPointLight ?? false);
  const [toolAreaLight, setToolAreaLight] = useState(storedState.toolAreaLight ?? false);
  const [toolConeLight, setToolConeLight] = useState(storedState.toolConeLight ?? false);
  const [toolWall, setToolWall] = useState(storedState.toolWall ?? false);
  const [toolCircle, setToolCircle] = useState(storedState.toolCircle ?? false);
  const [toolDrawShape, setToolDrawShape] = useState(storedState.toolDrawShape ?? false);
  const [toolLens, setToolLens] = useState(storedState.toolLens ?? false);
  const [toolMoveOptics, setToolMoveOptics] = useState(storedState.toolMoveOptics ?? false);
  const [toolEraseOptics, setToolEraseOptics] = useState(storedState.toolEraseOptics ?? false);
  const [toolSelectOptics, setToolSelectOptics] = useState(storedState.toolSelectOptics ?? false);
  const [trailsLength, setTrailsLength] = useState(storedState.trailsLength ?? defaultTrailsLength);
  const [trailsThickness, setTrailsThickness] = useState(storedState.trailsThickness ?? 0.1);
  const [globalTrails, setGlobalTrails] = useState(storedState.globalTrails ?? false);
  const [selectedTrails, setSelectedTrails] = useState(storedState.selectedTrails ?? false);
  const [localTrails, setLocalTrails] = useState(storedState.localTrails ?? false);
  const [whiteTrails, setWhiteTrails] = useState(storedState.whiteTrails ?? false);
  const [colorMode, setColorMode] = useState(storedState.colorMode ?? defaultColorMode);
  const [pauseAfterRecording, setPauseAfterRecording] = useState(storedState.pauseAfterRecording ?? false);
  const [cleanSceneAfterRecording, setCleanSceneAfterRecording] = useState(storedState.cleanSceneAfterRecording ?? false);
  const [recordingTimeLimit, setRecordingTimeLimit] = useState(storedState.recordingTimeLimit ?? 0);
  const [recordingMode, setRecordingMode] = useState(storedState.recordingMode ?? "webm");
  const [recordingFps, setRecordingFps] = useState(storedState.recordingFps ?? 60);
  const [showBrushCursor, setShowBrushCursor] = useState(storedState.showBrushCursor ?? true);
  const [hideBrushDuringRecording, setHideBrushDuringRecording] = useState(
    storedState.hideBrushDuringRecording ?? true
  );
  const [isRecording, setIsRecording] = useState(false);
  const mediaRecorderRef = useRef(null);
  const recordingChunksRef = useRef([]);
  const recordingTimerRef = useRef(null);
  const recordingModeRef = useRef(recordingMode);
  const brushCursorBeforeRecordingRef = useRef(null);
  const frameCaptureRef = useRef({
    active: false,
    stopping: false,
    frames: [],
    frameIndex: 0,
    maxFrames: null,
    inFlight: false,
    pendingResolve: null
  });
  const [toolEraser, setToolEraser] = useState(storedState.toolEraser ?? false);
  const [toolRadialForce, setToolRadialForce] = useState(storedState.toolRadialForce ?? false);
  const [toolSpin, setToolSpin] = useState(storedState.toolSpin ?? false);
  const [toolGrab, setToolGrab] = useState(storedState.toolGrab ?? false);
  const [showControls, setShowControls] = useState(storedState.showControls ?? false);
  const [showInfo, setShowInfo] = useState(storedState.showInfo ?? false);
  const [showTools, setShowTools] = useState(storedState.showTools ?? true);
  const [showParameters, setShowParameters] = useState(storedState.showParameters ?? true);
  const [showStats, setShowStats] = useState(storedState.showStats ?? true);
  const [parametersCollapsed, setParametersCollapsed] = useState(storedState.parametersCollapsed ?? false);
  const [toolsCollapsed, setToolsCollapsed] = useState(storedState.toolsCollapsed ?? false);
  const [settingsCollapsed, setSettingsCollapsed] = useState(storedState.settingsCollapsed ?? true);
  const [activeParamTab, setActiveParamTab] = useState(initialActiveTab);
  const [actionChoice, setActionChoice] = useState("");
  const hoverCount = useRef(0);
  const lightingProgress = maxSamples > 0 ? Math.min(lightingSamples / maxSamples, 1) : 0;

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

  const leftTabs = LEFT_TABS;

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

  const clearRecordingTimer = () => {
    if (recordingTimerRef.current) {
      clearTimeout(recordingTimerRef.current);
      recordingTimerRef.current = null;
    }
  };

  const finishRecording = () => {
    setIsRecording(false);
    if (hideBrushDuringRecording && brushCursorBeforeRecordingRef.current !== null) {
      const previous = brushCursorBeforeRecordingRef.current;
      brushCursorBeforeRecordingRef.current = null;
      setShowBrushCursor(previous);
      api?.setShowBrushCursor(previous ? 1 : 0);
    }
    if (pauseAfterRecording) {
      setTimePlaying(false);
      api?.setTimePlaying(0);
    }
    if (cleanSceneAfterRecording) {
      api?.clearScene();
    }
  };

  const applyBrushForRecording = () => {
    if (!hideBrushDuringRecording) return;
    if (brushCursorBeforeRecordingRef.current === null) {
      brushCursorBeforeRecordingRef.current = showBrushCursor;
    }
    if (showBrushCursor) {
      setShowBrushCursor(false);
      api?.setShowBrushCursor(0);
    }
  };

  const stopWebmRecording = () => {
    clearRecordingTimer();
    if (mediaRecorderRef.current && mediaRecorderRef.current.state !== "inactive") {
      mediaRecorderRef.current.stop();
    }
  };

  const startWebmRecording = () => {
    const canvas = document.getElementById("canvas");
    if (!canvas || !window.MediaRecorder) return;
    if (mediaRecorderRef.current && mediaRecorderRef.current.state !== "inactive") return;

    applyBrushForRecording();

    const fps = Math.max(1, Math.min(240, Math.round(recordingFps)));
    const stream = canvas.captureStream(fps);
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
      setTimeout(() => URL.revokeObjectURL(url), 1000);
      recordingChunksRef.current = [];
      finishRecording();
    };

    recorder.start();
    mediaRecorderRef.current = recorder;
    setIsRecording(true);

    if (recordingTimeLimit > 0) {
      recordingTimerRef.current = setTimeout(() => {
        stopWebmRecording();
      }, recordingTimeLimit * 1000);
    }
  };

  const getFrameLimit = () => {
    if (recordingTimeLimit <= 0) return null;
    const fps = Math.max(1, Math.min(240, Math.round(recordingFps)));
    return Math.max(1, Math.round(recordingTimeLimit * fps));
  };

  const startFrameCapture = () => {
    const canvas = document.getElementById("canvas");
    if (!canvas || !canvas.toBlob) return;
    if (frameCaptureRef.current.active) return;

    applyBrushForRecording();

    const capture = frameCaptureRef.current;
    capture.active = true;
    capture.stopping = false;
    capture.frames = [];
    capture.frameIndex = 0;
    capture.maxFrames = getFrameLimit();
    capture.inFlight = false;
    capture.pendingResolve = null;
    setIsRecording(true);

    window.webFrameCaptureTick = () => {
      const current = frameCaptureRef.current;
      if (!current.active || current.stopping || current.inFlight) return;
      current.inFlight = true;
      canvas.toBlob(
        (blob) => {
          const latest = frameCaptureRef.current;
          latest.inFlight = false;
          if (latest.pendingResolve) {
            const resolve = latest.pendingResolve;
            latest.pendingResolve = null;
            resolve();
          }
          if (!latest.active || latest.stopping) return;
          if (blob) {
            const frameNumber = String(latest.frameIndex + 1).padStart(6, "0");
            latest.frames.push({ name: `frame_${frameNumber}.png`, blob });
            latest.frameIndex += 1;
          }
          if (latest.maxFrames && latest.frameIndex >= latest.maxFrames) {
            stopFrameCapture();
          }
        },
        "image/png"
      );
    };
  };

  const stopFrameCapture = async () => {
    clearRecordingTimer();
    const capture = frameCaptureRef.current;
    if (!capture.active || capture.stopping) return;
    capture.active = false;
    capture.stopping = true;
    window.webFrameCaptureTick = null;
    if (capture.inFlight) {
      await new Promise((resolve) => {
        capture.pendingResolve = resolve;
      });
    }

    const canvas = document.getElementById("canvas");
    const fps = Math.max(1, Math.min(240, Math.round(recordingFps)));
    const metadata = {
      fps,
      width: canvas ? canvas.width : 0,
      height: canvas ? canvas.height : 0,
      frameCount: capture.frames.length,
      createdAt: new Date().toISOString()
    };
    const metadataBlob = new Blob([JSON.stringify(metadata, null, 2)], { type: "application/json" });
    const tar = await buildTar([{ name: "metadata.json", blob: metadataBlob }, ...capture.frames]);
    const url = URL.createObjectURL(tar);
    const anchor = document.createElement("a");
    const timestamp = new Date().toISOString().replace(/[:.]/g, "-");
    anchor.href = url;
    anchor.download = `galaxy-engine-frames-${timestamp}.tar`;
    anchor.click();
    setTimeout(() => URL.revokeObjectURL(url), 1000);

    capture.frames = [];
    capture.frameIndex = 0;
    capture.maxFrames = null;
    capture.inFlight = false;
    capture.pendingResolve = null;
    capture.stopping = false;
    finishRecording();
  };

  const startRecording = () => {
    recordingModeRef.current = recordingMode;
    if (recordingMode === "frames") {
      startFrameCapture();
    } else {
      startWebmRecording();
    }
  };

  const stopRecording = () => {
    if (recordingModeRef.current === "frames") {
      stopFrameCapture();
    } else {
      stopWebmRecording();
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

  const ensureTrailsVisible = () => {
    if (!api) return;
    let nextLength = trailsLength;
    let nextThickness = trailsThickness;
    if (nextLength <= 0) {
      nextLength = defaultTrailsLength;
      setTrailsLength(nextLength);
      api.setTrailsLength(nextLength);
    }
    if (nextThickness <= 0) {
      nextThickness = 0.1;
      setTrailsThickness(nextThickness);
      api.setTrailsThickness(nextThickness);
    }
  };

  useEffect(() => {
    if (!api) return;

    const stored = loadUiState() || {};
    const hasStored = Object.keys(stored).length > 0;
    const getBool = (key, fallback) =>
      typeof stored[key] === "boolean" ? stored[key] : fallback;
    const getNum = (key, fallback) =>
      Number.isFinite(stored[key]) ? stored[key] : fallback;
    const getColor = (key, fallback) => {
      const value = stored[key];
      if (
        value &&
        typeof value === "object" &&
        Number.isFinite(value.r) &&
        Number.isFinite(value.g) &&
        Number.isFinite(value.b) &&
        Number.isFinite(value.a)
      ) {
        return normalizeColor(value);
      }
      return normalizeColor(fallback);
    };

    const timePlayingValue = getBool("timePlaying", api.getTimePlaying() === 1);
    setTimePlaying(timePlayingValue);
    api.setTimePlaying(timePlayingValue ? 1 : 0);

    const timeStepValue = getNum("timeStep", api.getTimeStepMultiplier());
    setTimeStep(timeStepValue);
    api.setTimeStepMultiplier(timeStepValue);

    const targetFpsValue = getNum("targetFps", api.getTargetFps());
    setTargetFps(targetFpsValue);
    api.setTargetFps(targetFpsValue);

    const gravityValue = getNum("gravity", api.getGravityMultiplier());
    setGravity(gravityValue);
    api.setGravityMultiplier(gravityValue);

    const particleSizeValue = getNum("particleSizeMultiplier", api.getParticleSizeMultiplier());
    setParticleSizeMultiplier(particleSizeValue);
    api.setParticleSizeMultiplier(particleSizeValue);

    const thetaValue = getNum("theta", api.getTheta());
    setTheta(thetaValue);
    api.setTheta(thetaValue);

    const softeningValue = getNum("softening", api.getSoftening());
    setSoftening(softeningValue);
    api.setSoftening(softeningValue);

    const glowEnabledValue = getBool("glowEnabled", api.getGlowEnabled() === 1);
    setGlowEnabled(glowEnabledValue);
    api.setGlowEnabled(glowEnabledValue ? 1 : 0);

    const threadsAmountValue = getNum("threadsAmount", api.getThreadsAmount());
    setThreadsAmount(threadsAmountValue);
    api.setThreadsAmount(threadsAmountValue);

    const domainWidthValue = getNum("domainWidth", api.getDomainWidth());
    setDomainWidth(domainWidthValue);
    api.setDomainWidth(domainWidthValue);

    const domainHeightValue = getNum("domainHeight", api.getDomainHeight());
    setDomainHeight(domainHeightValue);
    api.setDomainHeight(domainHeightValue);

    const blackHoleInitMassValue = getNum("blackHoleInitMass", api.getBlackHoleInitMass());
    setBlackHoleInitMass(blackHoleInitMassValue);
    api.setBlackHoleInitMass(blackHoleInitMassValue);

    const ambientTemperatureValue = getNum("ambientTemperature", api.getAmbientTemperature());
    setAmbientTemperature(ambientTemperatureValue);
    api.setAmbientTemperature(ambientTemperatureValue);

    const ambientHeatRateValue = getNum("ambientHeatRate", api.getAmbientHeatRate());
    setAmbientHeatRate(ambientHeatRateValue);
    api.setAmbientHeatRate(ambientHeatRateValue);

    const heatConductivityValue = getNum("heatConductivity", api.getHeatConductivity());
    setHeatConductivity(heatConductivityValue);
    api.setHeatConductivity(heatConductivityValue);

    const constraintStiffnessValue = getNum("constraintStiffness", api.getConstraintStiffness());
    setConstraintStiffness(constraintStiffnessValue);
    api.setConstraintStiffness(constraintStiffnessValue);

    const constraintResistanceValue = getNum("constraintResistance", api.getConstraintResistance());
    setConstraintResistance(constraintResistanceValue);
    api.setConstraintResistance(constraintResistanceValue);

    const fluidVerticalGravityValue = getNum("fluidVerticalGravity", api.getFluidVerticalGravity());
    setFluidVerticalGravity(fluidVerticalGravityValue);
    api.setFluidVerticalGravity(fluidVerticalGravityValue);

    const fluidMassMultiplierValue = getNum("fluidMassMultiplier", api.getFluidMassMultiplier());
    setFluidMassMultiplier(fluidMassMultiplierValue);
    api.setFluidMassMultiplier(fluidMassMultiplierValue);

    const fluidViscosityValue = getNum("fluidViscosity", api.getFluidViscosity());
    setFluidViscosity(fluidViscosityValue);
    api.setFluidViscosity(fluidViscosityValue);

    const fluidStiffnessValue = getNum("fluidStiffness", api.getFluidStiffness());
    setFluidStiffness(fluidStiffnessValue);
    api.setFluidStiffness(fluidStiffnessValue);

    const fluidCohesionValue = getNum("fluidCohesion", api.getFluidCohesion());
    setFluidCohesion(fluidCohesionValue);
    api.setFluidCohesion(fluidCohesionValue);

    const fluidDeltaValue = getNum("fluidDelta", api.getFluidDelta());
    setFluidDelta(fluidDeltaValue);
    api.setFluidDelta(fluidDeltaValue);

    const fluidMaxVelocityValue = getNum("fluidMaxVelocity", api.getFluidMaxVelocity());
    setFluidMaxVelocity(fluidMaxVelocityValue);
    api.setFluidMaxVelocity(fluidMaxVelocityValue);

    const opticsEnabledValue = getBool("opticsEnabled", api.getOpticsEnabled() === 1);
    setOpticsEnabled(opticsEnabledValue);
    api.setOpticsEnabled(opticsEnabledValue ? 1 : 0);

    const lightGainValue = getNum("lightGain", api.getLightGain());
    setLightGain(lightGainValue);
    api.setLightGain(lightGainValue);

    const lightSpreadValue = getNum("lightSpread", api.getLightSpread());
    setLightSpread(lightSpreadValue);
    api.setLightSpread(lightSpreadValue);

    const wallSpecularRoughnessValue = getNum("wallSpecularRoughness", api.getWallSpecularRoughness());
    setWallSpecularRoughness(wallSpecularRoughnessValue);
    api.setWallSpecularRoughness(wallSpecularRoughnessValue);

    const wallRefractionRoughnessValue = getNum("wallRefractionRoughness", api.getWallRefractionRoughness());
    setWallRefractionRoughness(wallRefractionRoughnessValue);
    api.setWallRefractionRoughness(wallRefractionRoughnessValue);

    const wallRefractionAmountValue = getNum("wallRefractionAmount", api.getWallRefractionAmount());
    setWallRefractionAmount(wallRefractionAmountValue);
    api.setWallRefractionAmount(wallRefractionAmountValue);

    const wallIorValue = getNum("wallIor", api.getWallIor());
    setWallIor(wallIorValue);
    api.setWallIor(wallIorValue);

    const wallDispersionValue = getNum("wallDispersion", api.getWallDispersion());
    setWallDispersion(wallDispersionValue);
    api.setWallDispersion(wallDispersionValue);

    const wallEmissionGainValue = getNum("wallEmissionGain", api.getWallEmissionGain());
    setWallEmissionGain(wallEmissionGainValue);
    api.setWallEmissionGain(wallEmissionGainValue);

    const shapeRelaxIterValue = getNum("shapeRelaxIter", api.getShapeRelaxIter());
    setShapeRelaxIter(shapeRelaxIterValue);
    api.setShapeRelaxIter(shapeRelaxIterValue);

    const shapeRelaxFactorValue = getNum("shapeRelaxFactor", api.getShapeRelaxFactor());
    setShapeRelaxFactor(shapeRelaxFactorValue);
    api.setShapeRelaxFactor(shapeRelaxFactorValue);

    const maxSamplesValue = getNum("maxSamples", api.getMaxSamples());
    setMaxSamples(maxSamplesValue);
    api.setMaxSamples(maxSamplesValue);

    const sampleRaysAmountValue = getNum("sampleRaysAmount", api.getSampleRaysAmount());
    setSampleRaysAmount(sampleRaysAmountValue);
    api.setSampleRaysAmount(sampleRaysAmountValue);

    const maxBouncesValue = getNum("maxBounces", api.getMaxBounces());
    setMaxBounces(maxBouncesValue);
    api.setMaxBounces(maxBouncesValue);

    const diffuseEnabledValue = getBool("diffuseEnabled", api.getDiffuseEnabled() === 1);
    setDiffuseEnabled(diffuseEnabledValue);
    api.setDiffuseEnabled(diffuseEnabledValue ? 1 : 0);

    const specularEnabledValue = getBool("specularEnabled", api.getSpecularEnabled() === 1);
    setSpecularEnabled(specularEnabledValue);
    api.setSpecularEnabled(specularEnabledValue ? 1 : 0);

    const refractionEnabledValue = getBool("refractionEnabled", api.getRefractionEnabled() === 1);
    setRefractionEnabled(refractionEnabledValue);
    api.setRefractionEnabled(refractionEnabledValue ? 1 : 0);

    const dispersionEnabledValue = getBool("dispersionEnabled", api.getDispersionEnabled() === 1);
    setDispersionEnabled(dispersionEnabledValue);
    api.setDispersionEnabled(dispersionEnabledValue ? 1 : 0);

    const emissionEnabledValue = getBool("emissionEnabled", api.getEmissionEnabled() === 1);
    setEmissionEnabled(emissionEnabledValue);
    api.setEmissionEnabled(emissionEnabledValue ? 1 : 0);

    const symmetricalLensValue = getBool("symmetricalLens", api.getSymmetricalLens() === 1);
    setSymmetricalLens(symmetricalLensValue);
    api.setSymmetricalLens(symmetricalLensValue ? 1 : 0);

    const drawNormalsValue = getBool("drawNormals", api.getDrawNormals() === 1);
    setDrawNormals(drawNormalsValue);
    api.setDrawNormals(drawNormalsValue ? 1 : 0);

    const relaxMoveValue = getBool("relaxMove", api.getRelaxMove() === 1);
    setRelaxMove(relaxMoveValue);
    api.setRelaxMove(relaxMoveValue ? 1 : 0);

    const lightColorValue = getColor("lightColor", unpackColor(api.getLightColor()));
    setLightColor(lightColorValue);
    api.setLightColor(packColor(lightColorValue));

    const wallBaseColorValue = getColor("wallBaseColor", unpackColor(api.getWallBaseColor()));
    setWallBaseColor(wallBaseColorValue);
    api.setWallBaseColor(packColor(wallBaseColorValue));

    const wallSpecularColorValue = getColor("wallSpecularColor", unpackColor(api.getWallSpecularColor()));
    setWallSpecularColor(wallSpecularColorValue);
    api.setWallSpecularColor(packColor(wallSpecularColorValue));

    const wallRefractionColorValue = getColor("wallRefractionColor", unpackColor(api.getWallRefractionColor()));
    setWallRefractionColor(wallRefractionColorValue);
    api.setWallRefractionColor(packColor(wallRefractionColorValue));

    const wallEmissionColorValue = getColor("wallEmissionColor", unpackColor(api.getWallEmissionColor()));
    setWallEmissionColor(wallEmissionColorValue);
    api.setWallEmissionColor(packColor(wallEmissionColorValue));

    const lightingSamplesValue = api.getLightingSamples();
    setLightingSamples(lightingSamplesValue);

    const drawQuadtreeValue = getBool("drawQuadtree", api.getDrawQuadtree() === 1);
    setDrawQuadtree(drawQuadtreeValue);
    api.setDrawQuadtree(drawQuadtreeValue ? 1 : 0);

    const drawZCurvesValue = getBool("drawZCurves", api.getDrawZCurves() === 1);
    setDrawZCurves(drawZCurvesValue);
    api.setDrawZCurves(drawZCurvesValue ? 1 : 0);

    const toolDrawParticlesValue = getBool("toolDrawParticles", api.getToolDrawParticles() === 1);
    setToolDrawParticles(toolDrawParticlesValue);
    api.setToolDrawParticles(toolDrawParticlesValue ? 1 : 0);

    const toolBlackHoleValue = getBool("toolBlackHole", api.getToolBlackHole() === 1);
    setToolBlackHole(toolBlackHoleValue);
    api.setToolBlackHole(toolBlackHoleValue ? 1 : 0);

    const toolBigGalaxyValue = getBool("toolBigGalaxy", api.getToolBigGalaxy() === 1);
    setToolBigGalaxy(toolBigGalaxyValue);
    api.setToolBigGalaxy(toolBigGalaxyValue ? 1 : 0);

    const toolSmallGalaxyValue = getBool("toolSmallGalaxy", api.getToolSmallGalaxy() === 1);
    setToolSmallGalaxy(toolSmallGalaxyValue);
    api.setToolSmallGalaxy(toolSmallGalaxyValue ? 1 : 0);

    const toolStarValue = getBool("toolStar", api.getToolStar() === 1);
    setToolStar(toolStarValue);
    api.setToolStar(toolStarValue ? 1 : 0);

    const toolBigBangValue = getBool("toolBigBang", api.getToolBigBang() === 1);
    setToolBigBang(toolBigBangValue);
    api.setToolBigBang(toolBigBangValue ? 1 : 0);

    const toolPointLightValue = getBool("toolPointLight", api.getToolPointLight() === 1);
    setToolPointLight(toolPointLightValue);
    api.setToolPointLight(toolPointLightValue ? 1 : 0);

    const toolAreaLightValue = getBool("toolAreaLight", api.getToolAreaLight() === 1);
    setToolAreaLight(toolAreaLightValue);
    api.setToolAreaLight(toolAreaLightValue ? 1 : 0);

    const toolConeLightValue = getBool("toolConeLight", api.getToolConeLight() === 1);
    setToolConeLight(toolConeLightValue);
    api.setToolConeLight(toolConeLightValue ? 1 : 0);

    const toolWallValue = getBool("toolWall", api.getToolWall() === 1);
    setToolWall(toolWallValue);
    api.setToolWall(toolWallValue ? 1 : 0);

    const toolCircleValue = getBool("toolCircle", api.getToolCircle() === 1);
    setToolCircle(toolCircleValue);
    api.setToolCircle(toolCircleValue ? 1 : 0);

    const toolDrawShapeValue = getBool("toolDrawShape", api.getToolDrawShape() === 1);
    setToolDrawShape(toolDrawShapeValue);
    api.setToolDrawShape(toolDrawShapeValue ? 1 : 0);

    const toolLensValue = getBool("toolLens", api.getToolLens() === 1);
    setToolLens(toolLensValue);
    api.setToolLens(toolLensValue ? 1 : 0);

    const toolMoveOpticsValue = getBool("toolMoveOptics", api.getToolMoveOptics() === 1);
    setToolMoveOptics(toolMoveOpticsValue);
    api.setToolMoveOptics(toolMoveOpticsValue ? 1 : 0);

    const toolEraseOpticsValue = getBool("toolEraseOptics", api.getToolEraseOptics() === 1);
    setToolEraseOptics(toolEraseOpticsValue);
    api.setToolEraseOptics(toolEraseOpticsValue ? 1 : 0);

    const toolSelectOpticsValue = getBool("toolSelectOptics", api.getToolSelectOptics() === 1);
    setToolSelectOptics(toolSelectOpticsValue);
    api.setToolSelectOptics(toolSelectOpticsValue ? 1 : 0);

    const trailsLengthValue = getNum("trailsLength", hasStored ? defaultTrailsLength : api.getTrailsLength());
    let nextTrailsLength = trailsLengthValue;

    const trailsThicknessValue = getNum("trailsThickness", api.getTrailsThickness());
    let nextTrailsThickness = trailsThicknessValue;

    const globalTrailsValue = getBool("globalTrails", api.getGlobalTrails() === 1);
    setGlobalTrails(globalTrailsValue);
    api.setGlobalTrails(globalTrailsValue ? 1 : 0);

    const selectedTrailsValue = getBool("selectedTrails", api.getSelectedTrails() === 1);
    setSelectedTrails(selectedTrailsValue);
    api.setSelectedTrails(selectedTrailsValue ? 1 : 0);

    if ((globalTrailsValue || selectedTrailsValue) && nextTrailsLength <= 0) {
      nextTrailsLength = defaultTrailsLength;
    }
    if ((globalTrailsValue || selectedTrailsValue) && nextTrailsThickness <= 0) {
      nextTrailsThickness = 0.1;
    }

    setTrailsLength(nextTrailsLength);
    api.setTrailsLength(nextTrailsLength);

    setTrailsThickness(nextTrailsThickness);
    api.setTrailsThickness(nextTrailsThickness);

    const localTrailsValue = getBool("localTrails", api.getLocalTrails() === 1);
    setLocalTrails(localTrailsValue);
    api.setLocalTrails(localTrailsValue ? 1 : 0);

    const whiteTrailsValue = getBool("whiteTrails", api.getWhiteTrails() === 1);
    setWhiteTrails(whiteTrailsValue);
    api.setWhiteTrails(whiteTrailsValue ? 1 : 0);

    const colorModeValue = getNum("colorMode", hasStored ? defaultColorMode : api.getColorMode());
    setColorMode(colorModeValue);
    api.setColorMode(colorModeValue);

    const pauseAfterRecordingValue = getBool("pauseAfterRecording", api.getPauseAfterRecording() === 1);
    setPauseAfterRecording(pauseAfterRecordingValue);
    api.setPauseAfterRecording(pauseAfterRecordingValue ? 1 : 0);

    const cleanSceneAfterRecordingValue = getBool("cleanSceneAfterRecording", api.getCleanSceneAfterRecording() === 1);
    setCleanSceneAfterRecording(cleanSceneAfterRecordingValue);
    api.setCleanSceneAfterRecording(cleanSceneAfterRecordingValue ? 1 : 0);

    const recordingTimeLimitValue = getNum("recordingTimeLimit", api.getRecordingTimeLimit());
    setRecordingTimeLimit(recordingTimeLimitValue);
    api.setRecordingTimeLimit(recordingTimeLimitValue);

    const recordingModeValue =
      stored.recordingMode === "frames" || stored.recordingMode === "webm"
        ? stored.recordingMode
        : recordingMode;
    setRecordingMode(recordingModeValue);

    const recordingFpsValue = getNum("recordingFps", recordingFps);
    setRecordingFps(Math.max(1, Math.min(240, Math.round(recordingFpsValue))));

    const showBrushCursorValue = getBool("showBrushCursor", api.getShowBrushCursor() === 1);
    setShowBrushCursor(showBrushCursorValue);
    api.setShowBrushCursor(showBrushCursorValue ? 1 : 0);

    const hideBrushDuringRecordingValue = getBool(
      "hideBrushDuringRecording",
      hideBrushDuringRecording
    );
    setHideBrushDuringRecording(hideBrushDuringRecordingValue);

    const toolEraserValue = getBool("toolEraser", api.getToolEraser() === 1);
    setToolEraser(toolEraserValue);
    api.setToolEraser(toolEraserValue ? 1 : 0);

    const toolRadialForceValue = getBool("toolRadialForce", api.getToolRadialForce() === 1);
    setToolRadialForce(toolRadialForceValue);
    api.setToolRadialForce(toolRadialForceValue ? 1 : 0);

    const toolSpinValue = getBool("toolSpin", api.getToolSpin() === 1);
    setToolSpin(toolSpinValue);
    api.setToolSpin(toolSpinValue ? 1 : 0);

    const toolGrabValue = getBool("toolGrab", api.getToolGrab() === 1);
    setToolGrab(toolGrabValue);
    api.setToolGrab(toolGrabValue ? 1 : 0);

    const showControlsValue = getBool("showControls", showControls);
    setShowControls(showControlsValue);

    const showInfoValue = getBool("showInfo", showInfo);
    setShowInfo(showInfoValue);

    const showToolsValue = getBool("showTools", showTools);
    setShowTools(showToolsValue);

    const showParametersValue = getBool("showParameters", showParameters);
    setShowParameters(showParametersValue);

    const showStatsValue = getBool("showStats", showStats);
    setShowStats(showStatsValue);

    const parametersCollapsedValue = getBool("parametersCollapsed", parametersCollapsed);
    setParametersCollapsed(parametersCollapsedValue);

    const toolsCollapsedValue = getBool("toolsCollapsed", toolsCollapsed);
    setToolsCollapsed(toolsCollapsedValue);

    const settingsCollapsedValue = getBool("settingsCollapsed", settingsCollapsed);
    setSettingsCollapsed(settingsCollapsedValue);

    const activeTabValue = LEFT_TABS.includes(stored.activeParamTab)
      ? stored.activeParamTab
      : activeParamTab;
    setActiveParamTab(activeTabValue);

    api.setUiHover(0);
    window.webSetWindowSize = (width, height) => {
      api.setWindowSize(width, height);
    };
    if (window.webSyncCanvasSize) {
      window.webSyncCanvasSize();
    }
  }, [api]);

  useEffect(() => {
    saveUiState({
      timePlaying,
      timeStep,
      targetFps,
      gravity,
      particleSizeMultiplier,
      theta,
      softening,
      glowEnabled,
      threadsAmount,
      domainWidth,
      domainHeight,
      blackHoleInitMass,
      ambientTemperature,
      ambientHeatRate,
      heatConductivity,
      constraintStiffness,
      constraintResistance,
      fluidVerticalGravity,
      fluidMassMultiplier,
      fluidViscosity,
      fluidStiffness,
      fluidCohesion,
      fluidDelta,
      fluidMaxVelocity,
      opticsEnabled,
      lightGain,
      lightSpread,
      wallSpecularRoughness,
      wallRefractionRoughness,
      wallRefractionAmount,
      wallIor,
      wallDispersion,
      wallEmissionGain,
      shapeRelaxIter,
      shapeRelaxFactor,
      maxSamples,
      sampleRaysAmount,
      maxBounces,
      diffuseEnabled,
      specularEnabled,
      refractionEnabled,
      dispersionEnabled,
      emissionEnabled,
      symmetricalLens,
      drawNormals,
      relaxMove,
      lightColor,
      wallBaseColor,
      wallSpecularColor,
      wallRefractionColor,
      wallEmissionColor,
      drawQuadtree,
      drawZCurves,
      toolDrawParticles,
      toolBlackHole,
      toolBigGalaxy,
      toolSmallGalaxy,
      toolStar,
      toolBigBang,
      toolPointLight,
      toolAreaLight,
      toolConeLight,
      toolWall,
      toolCircle,
      toolDrawShape,
      toolLens,
      toolMoveOptics,
      toolEraseOptics,
      toolSelectOptics,
      trailsLength,
      trailsThickness,
      globalTrails,
      selectedTrails,
      localTrails,
      whiteTrails,
      colorMode,
      pauseAfterRecording,
      cleanSceneAfterRecording,
      recordingTimeLimit,
      recordingMode,
      recordingFps,
      showBrushCursor,
      hideBrushDuringRecording,
      toolEraser,
      toolRadialForce,
      toolSpin,
      toolGrab,
      showControls,
      showInfo,
      showTools,
      showParameters,
      showStats,
      parametersCollapsed,
      toolsCollapsed,
      settingsCollapsed,
    activeParamTab
  });
  }, [
    timePlaying,
    timeStep,
    targetFps,
    gravity,
    particleSizeMultiplier,
    theta,
    softening,
    glowEnabled,
    threadsAmount,
    domainWidth,
    domainHeight,
    blackHoleInitMass,
    ambientTemperature,
    ambientHeatRate,
    heatConductivity,
    constraintStiffness,
    constraintResistance,
    fluidVerticalGravity,
    fluidMassMultiplier,
    fluidViscosity,
    fluidStiffness,
    fluidCohesion,
    fluidDelta,
    fluidMaxVelocity,
    opticsEnabled,
    lightGain,
    lightSpread,
    wallSpecularRoughness,
    wallRefractionRoughness,
    wallRefractionAmount,
    wallIor,
    wallDispersion,
    wallEmissionGain,
    shapeRelaxIter,
    shapeRelaxFactor,
    maxSamples,
    sampleRaysAmount,
    maxBounces,
    diffuseEnabled,
    specularEnabled,
    refractionEnabled,
    dispersionEnabled,
    emissionEnabled,
    symmetricalLens,
    drawNormals,
    relaxMove,
    lightColor,
    wallBaseColor,
    wallSpecularColor,
    wallRefractionColor,
    wallEmissionColor,
    drawQuadtree,
    drawZCurves,
    toolDrawParticles,
    toolBlackHole,
    toolBigGalaxy,
    toolSmallGalaxy,
    toolStar,
    toolBigBang,
    toolPointLight,
    toolAreaLight,
    toolConeLight,
    toolWall,
    toolCircle,
    toolDrawShape,
    toolLens,
    toolMoveOptics,
    toolEraseOptics,
    toolSelectOptics,
    trailsLength,
    trailsThickness,
    globalTrails,
    selectedTrails,
    localTrails,
    whiteTrails,
    colorMode,
    pauseAfterRecording,
    cleanSceneAfterRecording,
    recordingTimeLimit,
    recordingMode,
    recordingFps,
    showBrushCursor,
    hideBrushDuringRecording,
    toolEraser,
    toolRadialForce,
    toolSpin,
    toolGrab,
    showControls,
    showInfo,
    showTools,
    showParameters,
    showStats,
    parametersCollapsed,
    toolsCollapsed,
    settingsCollapsed,
    activeParamTab
  ]);

  useEffect(() => {
    if (!api || (!opticsEnabled && activeParamTab !== "Optics")) return undefined;
    const intervalId = setInterval(() => {
      const samples = api.getLightingSamples();
      setLightingSamples(samples);
    }, 500);
    return () => clearInterval(intervalId);
  }, [api, opticsEnabled, activeParamTab]);

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
          <div
            style=${{
              pointerEvents: "auto",
              background: "rgba(12, 15, 22, 0.9)",
              border: "1px solid rgba(255,255,255,0.08)",
              borderRadius: 12,
              padding: 10
            }}
          >
            <div style=${{ display: "flex", flexWrap: "wrap", gap: 6 }}>
              ${leftTabs.map(
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
          </div>
          ${showParameters &&
          html`
            <${Panel}
              title="PARAMETERS"
              onHover=${updateHover}
              collapsed=${parametersCollapsed}
              onToggle=${() => setParametersCollapsed(!parametersCollapsed)}
              contentStyle=${{
                maxHeight: "70vh",
                overflow: "auto",
                paddingRight: 6
              }}
            >
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
                <${SectionTitle}>Particles</${SectionTitle}>
                <${Slider}
                  label="Particle Size"
                  value=${particleSizeMultiplier}
                  min=${0.1}
                  max=${5}
                  step=${0.05}
                  tooltip="Visual sharpness: larger sizes make particles softer and heavier on fill rate."
                  onChange=${(val) => {
                    setParticleSizeMultiplier(val);
                    api?.setParticleSizeMultiplier(val);
                  }}
                />
                <${SectionTitle}>Trails</${SectionTitle}>
                <${Toggle}
                  label="Global Trails"
                  value=${globalTrails}
                  onChange=${(val) => {
                    setGlobalTrails(val);
                    if (val) {
                      ensureTrailsVisible();
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
                      ensureTrailsVisible();
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
                <${LogIntSlider}
                  label="Trails Length"
                  value=${trailsLength}
                  max=${trailsLengthMax}
                  tooltip="Performance/clarity: longer trails are blurrier and cost more to render."
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
                  tooltip="Thickness affects sharpness and fill rate; thinner is faster and cleaner."
                  onChange=${(val) => {
                    setTrailsThickness(val);
                    api?.setTrailsThickness(val);
                  }}
                />
                <${SectionTitle}>Effects</${SectionTitle}>
                <${Toggle}
                  label="Glow"
                  value=${glowEnabled}
                  onChange=${(val) => {
                    setGlowEnabled(val);
                    api?.setGlowEnabled(val ? 1 : 0);
                  }}
                />
              `}
              ${activeParamTab === "Recording" &&
              html`
                <${SectionTitle}>Recording</${SectionTitle}>
                <label style=${{ display: "flex", flexDirection: "column", gap: 6, marginBottom: 12 }}>
                  <span>Recording Mode</span>
                  <select
                    value=${recordingMode}
                    disabled=${isRecording}
                    onChange=${(e) => setRecordingMode(e.target.value)}
                    style=${{
                      background: "rgba(255,255,255,0.06)",
                      border: "1px solid rgba(255,255,255,0.12)",
                      borderRadius: 8,
                      padding: "6px 8px",
                      color: "#e6edf3"
                    }}
                  >
                    <option value="webm">Real-time WebM (fast export)</option>
                    <option value="frames">Frame Capture (PNG sequence)</option>
                  </select>
                </label>
                <${IntSlider}
                  label="Recording FPS"
                  value=${recordingFps}
                  min=${1}
                  max=${120}
                  step=${1}
                  tooltip="Used for WebM capture or as playback FPS metadata for frame capture."
                  onChange=${(val) => {
                    if (isRecording) return;
                    setRecordingFps(val);
                  }}
                />
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
                  tooltip="WebM uses real seconds; frame capture uses this as final video length."
                  onChange=${(val) => {
                    if (isRecording) return;
                    setRecordingTimeLimit(val);
                    api?.setRecordingTimeLimit(val);
                  }}
                />
                <${Toggle}
                  label="Hide Brush During Recording"
                  value=${hideBrushDuringRecording}
                  onChange=${(val) => {
                    setHideBrushDuringRecording(val);
                  }}
                />
                <${Button}
                  label=${isRecording
                    ? recordingMode === "frames"
                      ? "Capturing Frames..."
                      : "Recording..."
                    : recordingMode === "frames"
                      ? "Start Frame Capture (PNG sequence)"
                      : `Start Recording (WebM @ ${Math.round(recordingFps)}fps)`}
                  onClick=${() => {
                    if (!isRecording) startRecording();
                  }}
                  disabled=${isRecording}
                />
                <${Button}
                  label=${recordingMode === "frames" ? "Stop Capture" : "Stop Recording"}
                  onClick=${() => {
                    if (isRecording) stopRecording();
                  }}
                  disabled=${!isRecording}
                />
                <${Note}>
                  ${recordingMode === "frames"
                    ? "Frame capture downloads a .tar with PNGs and metadata.json. Encode with ffmpeg at your chosen FPS."
                    : `Recording downloads a WebM file at ${Math.round(recordingFps)}fps.`}
                </${Note}>
              `}
              ${activeParamTab === "Optics" &&
              html`
                <${SectionTitle}>Optics</${SectionTitle}>
                <${Toggle}
                  label="Enable Optics"
                  value=${opticsEnabled}
                  onChange=${(val) => {
                    setOpticsEnabled(val);
                    api?.setOpticsEnabled(val ? 1 : 0);
                  }}
                />
                <${SectionTitle}>Light</${SectionTitle}>
                <${ColorPicker}
                  label="Light Color"
                  color=${lightColor}
                  onChange=${(next) => {
                    const normalized = normalizeColor(next);
                    setLightColor(normalized);
                    api?.setLightColor(packColor(normalized));
                  }}
                />
                <${Slider}
                  label="Light Gain"
                  value=${lightGain}
                  min=${0}
                  max=${1}
                  step=${0.01}
                  tooltip="Controls lights brightness."
                  onChange=${(val) => {
                    setLightGain(val);
                    api?.setLightGain(val);
                  }}
                />
                <${Slider}
                  label="Light Spread"
                  value=${lightSpread}
                  min=${0}
                  max=${1}
                  step=${0.01}
                  tooltip="Controls the spread of area and cone lights."
                  onChange=${(val) => {
                    setLightSpread(val);
                    api?.setLightSpread(val);
                  }}
                />
                <${SectionTitle}>Wall Colors</${SectionTitle}>
                <${ColorPicker}
                  label="Wall Base Color"
                  color=${wallBaseColor}
                  onChange=${(next) => {
                    const normalized = normalizeColor(next);
                    setWallBaseColor(normalized);
                    api?.setWallBaseColor(packColor(normalized));
                  }}
                />
                <${ColorPicker}
                  label="Wall Specular Color"
                  color=${wallSpecularColor}
                  onChange=${(next) => {
                    const normalized = normalizeColor(next);
                    setWallSpecularColor(normalized);
                    api?.setWallSpecularColor(packColor(normalized));
                  }}
                />
                <${ColorPicker}
                  label="Wall Refraction Color"
                  color=${wallRefractionColor}
                  onChange=${(next) => {
                    const normalized = normalizeColor(next);
                    setWallRefractionColor(normalized);
                    api?.setWallRefractionColor(packColor(normalized));
                  }}
                />
                <${ColorPicker}
                  label="Wall Emission Color"
                  color=${wallEmissionColor}
                  onChange=${(next) => {
                    const normalized = normalizeColor(next);
                    setWallEmissionColor(normalized);
                    api?.setWallEmissionColor(packColor(normalized));
                  }}
                />
                <${SectionTitle}>Wall Material</${SectionTitle}>
                <${Slider}
                  label="Wall Specular Roughness"
                  value=${wallSpecularRoughness}
                  min=${0}
                  max=${1}
                  step=${0.01}
                  tooltip="Controls the specular reflections roughness of walls."
                  onChange=${(val) => {
                    setWallSpecularRoughness(val);
                    api?.setWallSpecularRoughness(val);
                  }}
                />
                <${Slider}
                  label="Wall Refraction Roughness"
                  value=${wallRefractionRoughness}
                  min=${0}
                  max=${1}
                  step=${0.01}
                  tooltip="Controls the refraction surface roughness of walls."
                  onChange=${(val) => {
                    setWallRefractionRoughness(val);
                    api?.setWallRefractionRoughness(val);
                  }}
                />
                <${Slider}
                  label="Wall Refraction Amount"
                  value=${wallRefractionAmount}
                  min=${0}
                  max=${1}
                  step=${0.01}
                  tooltip="Controls how much light walls will refract."
                  onChange=${(val) => {
                    setWallRefractionAmount(val);
                    api?.setWallRefractionAmount(val);
                  }}
                />
                <${Slider}
                  label="Wall IOR"
                  value=${wallIor}
                  min=${0}
                  max=${100}
                  step=${0.1}
                  tooltip="Controls the IOR of walls."
                  onChange=${(val) => {
                    setWallIor(val);
                    api?.setWallIor(val);
                  }}
                />
                <${Slider}
                  label="Wall Dispersion"
                  value=${wallDispersion}
                  min=${0}
                  max=${0.2}
                  step=${0.001}
                  tooltip="Controls how much light gets dispersed after refracting from this wall."
                  onChange=${(val) => {
                    setWallDispersion(val);
                    api?.setWallDispersion(val);
                  }}
                />
                <${Slider}
                  label="Wall Emission Gain"
                  value=${wallEmissionGain}
                  min=${0}
                  max=${1}
                  step=${0.01}
                  tooltip="Controls how much light walls emit."
                  onChange=${(val) => {
                    setWallEmissionGain(val);
                    api?.setWallEmissionGain(val);
                  }}
                />
                <${SectionTitle}>Shape Settings</${SectionTitle}>
                <${IntSlider}
                  label="Shape Relax Iter."
                  value=${shapeRelaxIter}
                  min=${0}
                  max=${50}
                  step=${1}
                  tooltip="Controls the iterations used to relax the shapes when drawing."
                  onChange=${(val) => {
                    setShapeRelaxIter(val);
                    api?.setShapeRelaxIter(val);
                  }}
                />
                <${Slider}
                  label="Shape Relax Factor"
                  value=${shapeRelaxFactor}
                  min=${0}
                  max=${1}
                  step=${0.01}
                  tooltip="Controls how much the drawn shape should relax each iteration."
                  onChange=${(val) => {
                    setShapeRelaxFactor(val);
                    api?.setShapeRelaxFactor(val);
                  }}
                />
                <${SectionTitle}>Render Settings</${SectionTitle}>
                <${LogIntSlider}
                  label="Max Samples"
                  value=${maxSamples}
                  max=${2048}
                  tooltip="Controls the total amount of lighting iterations."
                  onChange=${(val) => {
                    setMaxSamples(val);
                    api?.setMaxSamples(val);
                  }}
                />
                <${LogIntSlider}
                  label="Rays Per Sample"
                  value=${sampleRaysAmount}
                  max=${8192}
                  tooltip="Controls amount of rays emitted on each sample."
                  onChange=${(val) => {
                    setSampleRaysAmount(val);
                    api?.setSampleRaysAmount(val);
                  }}
                />
                <${IntSlider}
                  label="Max Bounces"
                  value=${maxBounces}
                  min=${0}
                  max=${16}
                  step=${1}
                  tooltip="Controls how many times rays can bounce."
                  onChange=${(val) => {
                    setMaxBounces(val);
                    api?.setMaxBounces(val);
                  }}
                />
                <${SectionTitle}>Accumulation</${SectionTitle}>
                <div style=${{ display: "flex", justifyContent: "space-between", marginBottom: 8 }}>
                  <span>Samples</span>
                  <span style=${{ color: "#9aa4b2" }}>${Math.max(0, lightingSamples)} / ${maxSamples}</span>
                </div>
                <div
                  style=${{
                    height: 6,
                    borderRadius: 999,
                    background: "rgba(255,255,255,0.08)",
                    overflow: "hidden",
                    marginBottom: 10
                  }}
                >
                  <div
                    style=${{
                      height: "100%",
                      width: `${(lightingProgress * 100).toFixed(1)}%`,
                      background: "rgba(159,178,255,0.6)"
                    }}
                  />
                </div>
                <${Button}
                  label="Reset Accumulation"
                  onClick=${() => {
                    api?.resetLightingSamples();
                    setLightingSamples(0);
                  }}
                />
                <${SectionTitle}>Ray Features</${SectionTitle}>
                <${Toggle}
                  label="Global Illumination"
                  value=${diffuseEnabled}
                  onChange=${(val) => {
                    setDiffuseEnabled(val);
                    api?.setDiffuseEnabled(val ? 1 : 0);
                  }}
                />
                <${Toggle}
                  label="Specular Reflections"
                  value=${specularEnabled}
                  onChange=${(val) => {
                    setSpecularEnabled(val);
                    api?.setSpecularEnabled(val ? 1 : 0);
                  }}
                />
                <${Toggle}
                  label="Refraction"
                  value=${refractionEnabled}
                  onChange=${(val) => {
                    setRefractionEnabled(val);
                    api?.setRefractionEnabled(val ? 1 : 0);
                  }}
                />
                <${Toggle}
                  label="Dispersion"
                  value=${dispersionEnabled}
                  onChange=${(val) => {
                    setDispersionEnabled(val);
                    api?.setDispersionEnabled(val ? 1 : 0);
                  }}
                />
                <${Toggle}
                  label="Emission"
                  value=${emissionEnabled}
                  onChange=${(val) => {
                    setEmissionEnabled(val);
                    api?.setEmissionEnabled(val ? 1 : 0);
                  }}
                />
                <${SectionTitle}>Misc</${SectionTitle}>
                <${Toggle}
                  label="Symmetrical Lens"
                  value=${symmetricalLens}
                  onChange=${(val) => {
                    setSymmetricalLens(val);
                    api?.setSymmetricalLens(val ? 1 : 0);
                  }}
                />
                <${Toggle}
                  label="Show Normals"
                  value=${drawNormals}
                  onChange=${(val) => {
                    setDrawNormals(val);
                    api?.setDrawNormals(val ? 1 : 0);
                  }}
                />
                <${Toggle}
                  label="Relax Shape When Moved"
                  value=${relaxMove}
                  onChange=${(val) => {
                    setRelaxMove(val);
                    api?.setRelaxMove(val ? 1 : 0);
                  }}
                />
              `}
              ${activeParamTab === "Physics" &&
              html`
                <${SectionTitle}>System</${SectionTitle}>
                <${IntSlider}
                  label="Threads Amount"
                  value=${threadsAmount}
                  min=${1}
                  max=${32}
                  step=${1}
                  tooltip="Controls the amount of threads used by the simulation. Half your total amount of threads is usually the sweet spot."
                  onChange=${(val) => {
                    setThreadsAmount(val);
                    api?.setThreadsAmount(val);
                  }}
                />
                <${SectionTitle}>Simulation</${SectionTitle}>
                <${Slider}
                  label="Theta"
                  value=${theta}
                  min=${0.1}
                  max=${5}
                  step=${0.05}
                  tooltip="Controls the quality of the gravity calculation. Higher means lower quality."
                  onChange=${(val) => {
                    setTheta(val);
                    api?.setTheta(val);
                  }}
                />
                <${IntSlider}
                  label="Domain Width"
                  value=${domainWidth}
                  min=${200}
                  max=${3840}
                  step=${10}
                  tooltip="Controls the width of the global container."
                  onChange=${(val) => {
                    setDomainWidth(val);
                    api?.setDomainWidth(val);
                  }}
                />
                <${IntSlider}
                  label="Domain Height"
                  value=${domainHeight}
                  min=${200}
                  max=${2160}
                  step=${10}
                  tooltip="Controls the height of the global container."
                  onChange=${(val) => {
                    setDomainHeight(val);
                    api?.setDomainHeight(val);
                  }}
                />
                <${SectionTitle}>General Physics</${SectionTitle}>
                <${Slider}
                  label="Time Scale"
                  value=${timeStep}
                  min=${0}
                  max=${15}
                  step=${0.05}
                  tooltip="Controls how fast time passes."
                  onChange=${(val) => {
                    setTimeStep(val);
                    api?.setTimeStepMultiplier(val);
                  }}
                />
                <${Slider}
                  label="Softening"
                  value=${softening}
                  min=${0.5}
                  max=${30}
                  step=${0.1}
                  tooltip="Controls the smoothness of the gravity forces."
                  onChange=${(val) => {
                    setSoftening(val);
                    api?.setSoftening(val);
                  }}
                />
                <${Slider}
                  label="Gravity Strength"
                  value=${gravity}
                  min=${0}
                  max=${100}
                  step=${0.1}
                  tooltip="Controls how much particles attract eachother."
                  onChange=${(val) => {
                    setGravity(val);
                    api?.setGravityMultiplier(val);
                  }}
                />
                <${Slider}
                  label="Black Hole Init Mass"
                  value=${blackHoleInitMass}
                  min=${0.005}
                  max=${15}
                  step=${0.005}
                  tooltip="Controls the mass of black holes when spawned."
                  onChange=${(val) => {
                    setBlackHoleInitMass(val);
                    api?.setBlackHoleInitMass(val);
                  }}
                />
                <${SectionTitle}>Temperature</${SectionTitle}>
                <${IntSlider}
                  label="Ambient Temperature"
                  value=${ambientTemperature}
                  min=${1}
                  max=${2500}
                  step=${1}
                  tooltip="Controls the desired temperature of the scene in Kelvin. 1 is near absolute zero. The default value is set just high enough to allow liquid water."
                  onChange=${(val) => {
                    setAmbientTemperature(val);
                    api?.setAmbientTemperature(val);
                  }}
                />
                <${Slider}
                  label="Ambient Heat Rate"
                  value=${ambientHeatRate}
                  min=${0}
                  max=${10}
                  step=${0.05}
                  tooltip="Controls how fast particles' temperature try to match ambient temperature."
                  onChange=${(val) => {
                    setAmbientHeatRate(val);
                    api?.setAmbientHeatRate(val);
                  }}
                />
                <${Slider}
                  label="Heat Conductivity Multiplier"
                  value=${heatConductivity}
                  min=${0.001}
                  max=${1}
                  step=${0.001}
                  tooltip="Controls the global heat conductivity of particles."
                  onChange=${(val) => {
                    setHeatConductivity(val);
                    api?.setHeatConductivity(val);
                  }}
                />
                <${SectionTitle}>Constraints</${SectionTitle}>
                <${Slider}
                  label="Constraints Stiffness Multiplier"
                  value=${constraintStiffness}
                  min=${0.001}
                  max=${3}
                  step=${0.001}
                  tooltip="Controls the global stiffness multiplier for constraints."
                  onChange=${(val) => {
                    setConstraintStiffness(val);
                    api?.setConstraintStiffness(val);
                  }}
                />
                <${Slider}
                  label="Constraints Resistance Multiplier"
                  value=${constraintResistance}
                  min=${0.001}
                  max=${30}
                  step=${0.01}
                  tooltip="Controls the global resistance multiplier for constraints."
                  onChange=${(val) => {
                    setConstraintResistance(val);
                    api?.setConstraintResistance(val);
                  }}
                />
                <${SectionTitle}>Fluids</${SectionTitle}>
                <${Slider}
                  label="Fluid Vertical Gravity"
                  value=${fluidVerticalGravity}
                  min=${0}
                  max=${10}
                  step=${0.1}
                  tooltip="Controls the vertical gravity strength in Fluid Ground Mode."
                  onChange=${(val) => {
                    setFluidVerticalGravity(val);
                    api?.setFluidVerticalGravity(val);
                  }}
                />
                <${Slider}
                  label="Fluid Mass Multiplier"
                  value=${fluidMassMultiplier}
                  min=${0.005}
                  max=${0.15}
                  step=${0.001}
                  tooltip="Controls the fluid mass of particles."
                  onChange=${(val) => {
                    setFluidMassMultiplier(val);
                    api?.setFluidMassMultiplier(val);
                  }}
                />
                <${Slider}
                  label="Fluid Viscosity"
                  value=${fluidViscosity}
                  min=${0.01}
                  max=${15}
                  step=${0.01}
                  tooltip="Controls how viscous particles are."
                  onChange=${(val) => {
                    setFluidViscosity(val);
                    api?.setFluidViscosity(val);
                  }}
                />
                <${Slider}
                  label="Fluid Stiffness"
                  value=${fluidStiffness}
                  min=${0.01}
                  max=${15}
                  step=${0.01}
                  tooltip="Controls how stiff particles are."
                  onChange=${(val) => {
                    setFluidStiffness(val);
                    api?.setFluidStiffness(val);
                  }}
                />
                <${Slider}
                  label="Fluid Cohesion"
                  value=${fluidCohesion}
                  min=${0}
                  max=${10}
                  step=${0.01}
                  tooltip="Controls how sticky particles are."
                  onChange=${(val) => {
                    setFluidCohesion(val);
                    api?.setFluidCohesion(val);
                  }}
                />
                <${Slider}
                  label="Fluid Delta"
                  value=${fluidDelta}
                  min=${500}
                  max=${20000}
                  step=${50}
                  tooltip="Controls the scaling factor in the pressure solver to enforce fluid incompressibility."
                  onChange=${(val) => {
                    setFluidDelta(val);
                    api?.setFluidDelta(val);
                  }}
                />
                <${Slider}
                  label="Fluid Max Velocity"
                  value=${fluidMaxVelocity}
                  min=${0}
                  max=${2000}
                  step=${10}
                  tooltip="Controls the maximum velocity a particle can have in Fluid mode."
                  onChange=${(val) => {
                    setFluidMaxVelocity(val);
                    api?.setFluidMaxVelocity(val);
                  }}
                />
              `}
              ${activeParamTab !== "Visuals" &&
              activeParamTab !== "Recording" &&
              activeParamTab !== "Optics" &&
              activeParamTab !== "Physics" &&
              html`<${Note}>${activeParamTab} parameters are not fully wired yet.</${Note}>`}
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
          ${showTools &&
          html`
            <${Panel}
              title="CURRENT TOOL"
              onHover=${updateHover}
              collapsed=${toolsCollapsed}
              onToggle=${() => setToolsCollapsed(!toolsCollapsed)}
              style=${{
                pointerEvents: "auto",
                display: "flex",
                flexDirection: "column",
                minHeight: 0
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
          <${Panel}
            title="SETTINGS"
            onHover=${updateHover}
            collapsed=${settingsCollapsed}
            onToggle=${() => setSettingsCollapsed(!settingsCollapsed)}
            style=${{ pointerEvents: "auto", maxHeight: "60vh", overflow: "auto" }}
          >
            <${SectionTitle}>Panels</${SectionTitle}>
            <${Toggle} label="Show Controls" value=${showControls} onChange=${setShowControls} />
            <${Toggle} label="Show Information" value=${showInfo} onChange=${setShowInfo} />
            <${Toggle} label="Show Tools" value=${showTools} onChange=${setShowTools} />
            <${Toggle} label="Show Parameters" value=${showParameters} onChange=${setShowParameters} />
            <${Toggle} label="Show Stats" value=${showStats} onChange=${setShowStats} />

            <${SectionTitle}>Simulation</${SectionTitle}>
            <${Slider}
              label="Time Step"
              value=${timeStep}
              min=${0}
              max=${15}
              step=${0.05}
              tooltip="Controls how fast time passes."
              onChange=${(val) => {
                setTimeStep(val);
                api?.setTimeStepMultiplier(val);
              }}
            />
            <${SectionTitle}>Brush</${SectionTitle}>
            <${Toggle}
              label="Show Brush Cursor"
              value=${showBrushCursor}
              onChange=${(val) => {
                setShowBrushCursor(val);
                api?.setShowBrushCursor(val ? 1 : 0);
              }}
            />
            <${SectionTitle}>Physics</${SectionTitle}>
            <${Slider}
              label="Gravity Multiplier"
              value=${gravity}
              min=${0}
              max=${100}
              step=${0.1}
              tooltip="Controls how much particles attract eachother."
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
                title="Caps render FPS; lowering can reduce CPU/GPU load."
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

      <div
        style=${{
          position: "absolute",
          left: "50%",
          bottom: 28,
          transform: "translateX(-50%)",
          zIndex: 3,
          pointerEvents: "auto"
        }}
      >
        <button
          onClick=${() => {
            const next = !timePlaying;
            setTimePlaying(next);
            api?.setTimePlaying(next ? 1 : 0);
          }}
          style=${{
            display: "flex",
            alignItems: "center",
            gap: 10,
            padding: "10px 18px",
            borderRadius: 999,
            border: "1px solid rgba(255,255,255,0.16)",
            background: timePlaying ? "rgba(255,255,255,0.08)" : "rgba(159,178,255,0.2)",
            color: "#e6edf3",
            letterSpacing: "0.08em",
            textTransform: "uppercase",
            fontSize: 12,
            cursor: "pointer",
            boxShadow: "0 16px 30px rgba(0,0,0,0.35)"
          }}
        >
          ${timePlaying ? "Pause" : "Play"}
        </button>
      </div>

    </${React.Fragment}>
  `;
}

root.render(html`<${App} />`);
