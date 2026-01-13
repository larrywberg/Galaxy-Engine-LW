#include "globalLogic.h"
#include "parameters.h"

#include <emscripten/emscripten.h>

extern UpdateVariables myVar;
extern UpdateParameters myParam;

extern "C" {

EMSCRIPTEN_KEEPALIVE void web_set_time_playing(int playing) {
	myVar.isTimePlaying = (playing != 0);
}

EMSCRIPTEN_KEEPALIVE int web_get_time_playing() {
	return myVar.isTimePlaying ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_time_factor(float factor) {
	myVar.timeFactor = factor;
}

EMSCRIPTEN_KEEPALIVE float web_get_time_factor() {
	return myVar.timeFactor;
}

EMSCRIPTEN_KEEPALIVE void web_set_target_fps(int fps) {
	if (fps < 1) {
		fps = 1;
	}
	myVar.targetFPS = fps;
	SetTargetFPS(fps);
}

EMSCRIPTEN_KEEPALIVE int web_get_target_fps() {
	return myVar.targetFPS;
}

EMSCRIPTEN_KEEPALIVE void web_set_gravity_multiplier(float mult) {
	myVar.gravityMultiplier = mult;
}

EMSCRIPTEN_KEEPALIVE float web_get_gravity_multiplier() {
	return myVar.gravityMultiplier;
}

EMSCRIPTEN_KEEPALIVE void web_set_time_step_multiplier(float mult) {
	myVar.timeStepMultiplier = mult;
}

EMSCRIPTEN_KEEPALIVE float web_get_time_step_multiplier() {
	return myVar.timeStepMultiplier;
}

EMSCRIPTEN_KEEPALIVE void web_set_window_size(int width, int height) {
	if (width < 1) width = 1;
	if (height < 1) height = 1;
	myVar.screenWidth = width;
	myVar.screenHeight = height;
	myVar.halfScreenWidth = width * 0.5f;
	myVar.halfScreenHeight = height * 0.5f;
	SetWindowSize(width, height);
}

EMSCRIPTEN_KEEPALIVE void web_set_pause_after_recording(int enabled) {
	myVar.pauseAfterRecording = (enabled != 0);
}

EMSCRIPTEN_KEEPALIVE int web_get_pause_after_recording() {
	return myVar.pauseAfterRecording ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_clean_scene_after_recording(int enabled) {
	myVar.cleanSceneAfterRecording = (enabled != 0);
}

EMSCRIPTEN_KEEPALIVE int web_get_clean_scene_after_recording() {
	return myVar.cleanSceneAfterRecording ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_recording_time_limit(float seconds) {
	if (seconds < 0.0f) {
		seconds = 0.0f;
	}
	myVar.recordingTimeLimit = seconds;
}

EMSCRIPTEN_KEEPALIVE float web_get_recording_time_limit() {
	return myVar.recordingTimeLimit;
}

EMSCRIPTEN_KEEPALIVE void web_set_show_brush_cursor(int enabled) {
	myVar.showBrushCursor = (enabled != 0);
}

EMSCRIPTEN_KEEPALIVE int web_get_show_brush_cursor() {
	return myVar.showBrushCursor ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_clear_scene() {
	myParam.pParticles.clear();
	myParam.rParticles.clear();
	myParam.pParticlesSelected.clear();
	myParam.rParticlesSelected.clear();
	myParam.trails.segments.clear();
}

EMSCRIPTEN_KEEPALIVE void web_set_glow_enabled(int enabled) {
	myVar.isGlowEnabled = (enabled != 0);
}

EMSCRIPTEN_KEEPALIVE int web_get_glow_enabled() {
	return myVar.isGlowEnabled ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_trails_length(int length) {
	if (length < 0) {
		length = 0;
	}
	myVar.trailMaxLength = length;
}

EMSCRIPTEN_KEEPALIVE int web_get_trails_length() {
	return myVar.trailMaxLength;
}

EMSCRIPTEN_KEEPALIVE void web_set_trails_thickness(float thickness) {
	if (thickness < 0.0f) {
		thickness = 0.0f;
	}
	myParam.trails.trailThickness = thickness;
}

EMSCRIPTEN_KEEPALIVE float web_get_trails_thickness() {
	return myParam.trails.trailThickness;
}

EMSCRIPTEN_KEEPALIVE void web_set_global_trails(int enabled) {
	if (enabled) {
		myVar.isGlobalTrailsEnabled = true;
		myVar.isSelectedTrailsEnabled = false;
	} else {
		myVar.isGlobalTrailsEnabled = false;
	}
}

EMSCRIPTEN_KEEPALIVE int web_get_global_trails() {
	return myVar.isGlobalTrailsEnabled ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_selected_trails(int enabled) {
	if (enabled) {
		myVar.isSelectedTrailsEnabled = true;
		myVar.isGlobalTrailsEnabled = false;
	} else {
		myVar.isSelectedTrailsEnabled = false;
	}
}

EMSCRIPTEN_KEEPALIVE int web_get_selected_trails() {
	return myVar.isSelectedTrailsEnabled ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_local_trails(int enabled) {
	myVar.isLocalTrailsEnabled = (enabled != 0);
}

EMSCRIPTEN_KEEPALIVE int web_get_local_trails() {
	return myVar.isLocalTrailsEnabled ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_white_trails(int enabled) {
	myParam.trails.whiteTrails = (enabled != 0);
}

EMSCRIPTEN_KEEPALIVE int web_get_white_trails() {
	return myParam.trails.whiteTrails ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_color_mode(int mode) {
	myParam.colorVisuals.solidColor = false;
	myParam.colorVisuals.densityColor = false;
	myParam.colorVisuals.forceColor = false;
	myParam.colorVisuals.velocityColor = false;
	myParam.colorVisuals.shockwaveColor = false;
	myParam.colorVisuals.turbulenceColor = false;
	myParam.colorVisuals.pressureColor = false;
	myParam.colorVisuals.temperatureColor = false;
	myParam.colorVisuals.gasTempColor = false;
	myParam.colorVisuals.SPHColor = false;

	switch (mode) {
	case 0: myParam.colorVisuals.solidColor = true; break;
	case 1: myParam.colorVisuals.densityColor = true; break;
	case 2: myParam.colorVisuals.forceColor = true; break;
	case 3: myParam.colorVisuals.velocityColor = true; break;
	case 4: myParam.colorVisuals.shockwaveColor = true; break;
	case 5: myParam.colorVisuals.turbulenceColor = true; break;
	case 6: myParam.colorVisuals.pressureColor = true; break;
	case 7: myParam.colorVisuals.temperatureColor = true; break;
	case 8: myParam.colorVisuals.gasTempColor = true; break;
	case 9: myParam.colorVisuals.SPHColor = true; break;
	default: myParam.colorVisuals.solidColor = true; break;
	}
}

EMSCRIPTEN_KEEPALIVE int web_get_color_mode() {
	if (myParam.colorVisuals.solidColor) return 0;
	if (myParam.colorVisuals.densityColor) return 1;
	if (myParam.colorVisuals.forceColor) return 2;
	if (myParam.colorVisuals.velocityColor) return 3;
	if (myParam.colorVisuals.shockwaveColor) return 4;
	if (myParam.colorVisuals.turbulenceColor) return 5;
	if (myParam.colorVisuals.pressureColor) return 6;
	if (myParam.colorVisuals.temperatureColor) return 7;
	if (myParam.colorVisuals.gasTempColor) return 8;
	if (myParam.colorVisuals.SPHColor) return 9;
	return 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_particle_size_multiplier(float mult) {
	if (mult < 0.1f) {
		mult = 0.1f;
	}
	if (mult > 5.0f) {
		mult = 5.0f;
	}
	myVar.particleSizeMultiplier = mult;
}

EMSCRIPTEN_KEEPALIVE float web_get_particle_size_multiplier() {
	return myVar.particleSizeMultiplier;
}

EMSCRIPTEN_KEEPALIVE void web_set_theta(float theta) {
	if (theta < 0.1f) {
		theta = 0.1f;
	}
	if (theta > 5.0f) {
		theta = 5.0f;
	}
	myVar.theta = theta;
}

EMSCRIPTEN_KEEPALIVE float web_get_theta() {
	return myVar.theta;
}

EMSCRIPTEN_KEEPALIVE void web_set_softening(float softening) {
	if (softening < 0.5f) {
		softening = 0.5f;
	}
	if (softening > 30.0f) {
		softening = 30.0f;
	}
	myVar.softening = softening;
}

EMSCRIPTEN_KEEPALIVE float web_get_softening() {
	return myVar.softening;
}

EMSCRIPTEN_KEEPALIVE void web_set_ui_hover(int hovering) {
	myVar.isMouseNotHoveringUI = (hovering == 0);
}

static void clear_all_tools() {
	myVar.toolSpawnHeavyParticle = false;
	myVar.toolDrawParticles = false;
	myVar.toolSpawnSmallGalaxy = false;
	myVar.toolSpawnBigGalaxy = false;
	myVar.toolSpawnStar = false;
	myVar.toolSpawnBigBang = false;
	myVar.toolErase = false;
	myVar.toolRadialForce = false;
	myVar.toolSpin = false;
	myVar.toolMove = false;
	myVar.toolRaiseTemp = false;
	myVar.toolLowerTemp = false;
	myVar.toolPointLight = false;
	myVar.toolAreaLight = false;
	myVar.toolConeLight = false;
	myVar.toolCircle = false;
	myVar.toolDrawShape = false;
	myVar.toolLens = false;
	myVar.toolWall = false;
	myVar.toolMoveOptics = false;
	myVar.toolEraseOptics = false;
	myVar.toolSelectOptics = false;
	myVar.longExposureFlag = false;
}

EMSCRIPTEN_KEEPALIVE void web_set_tool_draw_particles(int enabled) {
	if (enabled) {
		clear_all_tools();
		myVar.toolDrawParticles = true;
	} else {
		myVar.toolDrawParticles = false;
	}
}

EMSCRIPTEN_KEEPALIVE int web_get_tool_draw_particles() {
	return myVar.toolDrawParticles ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_tool_black_hole(int enabled) {
	if (enabled) {
		clear_all_tools();
		myVar.toolSpawnHeavyParticle = true;
	} else {
		myVar.toolSpawnHeavyParticle = false;
	}
}

EMSCRIPTEN_KEEPALIVE int web_get_tool_black_hole() {
	return myVar.toolSpawnHeavyParticle ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_tool_big_galaxy(int enabled) {
	if (enabled) {
		clear_all_tools();
		myVar.toolSpawnBigGalaxy = true;
	} else {
		myVar.toolSpawnBigGalaxy = false;
	}
}

EMSCRIPTEN_KEEPALIVE int web_get_tool_big_galaxy() {
	return myVar.toolSpawnBigGalaxy ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_tool_small_galaxy(int enabled) {
	if (enabled) {
		clear_all_tools();
		myVar.toolSpawnSmallGalaxy = true;
	} else {
		myVar.toolSpawnSmallGalaxy = false;
	}
}

EMSCRIPTEN_KEEPALIVE int web_get_tool_small_galaxy() {
	return myVar.toolSpawnSmallGalaxy ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_tool_star(int enabled) {
	if (enabled) {
		clear_all_tools();
		myVar.toolSpawnStar = true;
	} else {
		myVar.toolSpawnStar = false;
	}
}

EMSCRIPTEN_KEEPALIVE int web_get_tool_star() {
	return myVar.toolSpawnStar ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_tool_big_bang(int enabled) {
	if (enabled) {
		clear_all_tools();
		myVar.toolSpawnBigBang = true;
	} else {
		myVar.toolSpawnBigBang = false;
	}
}

EMSCRIPTEN_KEEPALIVE int web_get_tool_big_bang() {
	return myVar.toolSpawnBigBang ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_tool_point_light(int enabled) {
	if (enabled) {
		clear_all_tools();
		myVar.toolPointLight = true;
	} else {
		myVar.toolPointLight = false;
	}
}

EMSCRIPTEN_KEEPALIVE int web_get_tool_point_light() {
	return myVar.toolPointLight ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_tool_area_light(int enabled) {
	if (enabled) {
		clear_all_tools();
		myVar.toolAreaLight = true;
	} else {
		myVar.toolAreaLight = false;
	}
}

EMSCRIPTEN_KEEPALIVE int web_get_tool_area_light() {
	return myVar.toolAreaLight ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_tool_cone_light(int enabled) {
	if (enabled) {
		clear_all_tools();
		myVar.toolConeLight = true;
	} else {
		myVar.toolConeLight = false;
	}
}

EMSCRIPTEN_KEEPALIVE int web_get_tool_cone_light() {
	return myVar.toolConeLight ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_tool_wall(int enabled) {
	if (enabled) {
		clear_all_tools();
		myVar.toolWall = true;
	} else {
		myVar.toolWall = false;
	}
}

EMSCRIPTEN_KEEPALIVE int web_get_tool_wall() {
	return myVar.toolWall ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_tool_circle(int enabled) {
	if (enabled) {
		clear_all_tools();
		myVar.toolCircle = true;
	} else {
		myVar.toolCircle = false;
	}
}

EMSCRIPTEN_KEEPALIVE int web_get_tool_circle() {
	return myVar.toolCircle ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_tool_draw_shape(int enabled) {
	if (enabled) {
		clear_all_tools();
		myVar.toolDrawShape = true;
	} else {
		myVar.toolDrawShape = false;
	}
}

EMSCRIPTEN_KEEPALIVE int web_get_tool_draw_shape() {
	return myVar.toolDrawShape ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_tool_lens(int enabled) {
	if (enabled) {
		clear_all_tools();
		myVar.toolLens = true;
	} else {
		myVar.toolLens = false;
	}
}

EMSCRIPTEN_KEEPALIVE int web_get_tool_lens() {
	return myVar.toolLens ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_tool_move_optics(int enabled) {
	if (enabled) {
		clear_all_tools();
		myVar.toolMoveOptics = true;
	} else {
		myVar.toolMoveOptics = false;
	}
}

EMSCRIPTEN_KEEPALIVE int web_get_tool_move_optics() {
	return myVar.toolMoveOptics ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_tool_erase_optics(int enabled) {
	if (enabled) {
		clear_all_tools();
		myVar.toolEraseOptics = true;
	} else {
		myVar.toolEraseOptics = false;
	}
}

EMSCRIPTEN_KEEPALIVE int web_get_tool_erase_optics() {
	return myVar.toolEraseOptics ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_tool_select_optics(int enabled) {
	if (enabled) {
		clear_all_tools();
		myVar.toolSelectOptics = true;
	} else {
		myVar.toolSelectOptics = false;
	}
}

EMSCRIPTEN_KEEPALIVE int web_get_tool_select_optics() {
	return myVar.toolSelectOptics ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_tool_eraser(int enabled) {
	if (enabled) {
		clear_all_tools();
		myVar.toolErase = true;
	} else {
		myVar.toolErase = false;
	}
}

EMSCRIPTEN_KEEPALIVE int web_get_tool_eraser() {
	return myVar.toolErase ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_tool_radial_force(int enabled) {
	if (enabled) {
		clear_all_tools();
		myVar.toolRadialForce = true;
	} else {
		myVar.toolRadialForce = false;
	}
}

EMSCRIPTEN_KEEPALIVE int web_get_tool_radial_force() {
	return myVar.toolRadialForce ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_tool_spin(int enabled) {
	if (enabled) {
		clear_all_tools();
		myVar.toolSpin = true;
	} else {
		myVar.toolSpin = false;
	}
}

EMSCRIPTEN_KEEPALIVE int web_get_tool_spin() {
	return myVar.toolSpin ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_tool_grab(int enabled) {
	if (enabled) {
		clear_all_tools();
		myVar.toolMove = true;
	} else {
		myVar.toolMove = false;
	}
}

EMSCRIPTEN_KEEPALIVE int web_get_tool_grab() {
	return myVar.toolMove ? 1 : 0;
}

// Right-click actions (single-shot triggers)
EMSCRIPTEN_KEEPALIVE void web_rc_subdivide_all() { myParam.subdivision.subdivideAll = true; }
EMSCRIPTEN_KEEPALIVE void web_rc_subdivide_selected() { myParam.subdivision.subdivideSelected = true; }
EMSCRIPTEN_KEEPALIVE void web_rc_delete_selection() { myParam.particleDeletion.deleteSelection = true; }
EMSCRIPTEN_KEEPALIVE void web_rc_delete_stray() { myParam.particleDeletion.deleteNonImportant = true; }
EMSCRIPTEN_KEEPALIVE void web_rc_deselect_all() { myParam.particleSelection.deselectParticles = true; }
EMSCRIPTEN_KEEPALIVE void web_rc_invert_selection() { myParam.particleSelection.invertParticleSelection = true; }
EMSCRIPTEN_KEEPALIVE void web_rc_select_clusters() { myParam.particleSelection.selectManyClusters = true; }
EMSCRIPTEN_KEEPALIVE void web_rc_center_camera() { myParam.myCamera.centerCamera = true; }
EMSCRIPTEN_KEEPALIVE void web_rc_pin_selected() { myVar.pinFlag = true; }
EMSCRIPTEN_KEEPALIVE void web_rc_unpin_selected() { myVar.unPinFlag = true; }

// Right-click toggles
EMSCRIPTEN_KEEPALIVE void web_set_draw_quadtree(int enabled) { myVar.drawQuadtree = (enabled != 0); }
EMSCRIPTEN_KEEPALIVE int web_get_draw_quadtree() { return myVar.drawQuadtree ? 1 : 0; }
EMSCRIPTEN_KEEPALIVE void web_set_draw_z_curves(int enabled) { myVar.drawZCurves = (enabled != 0); }
EMSCRIPTEN_KEEPALIVE int web_get_draw_z_curves() { return myVar.drawZCurves ? 1 : 0; }

}
