#include "globalLogic.h"
#include "parameters.h"

#include <cstdint>
#include <emscripten/emscripten.h>

extern UpdateVariables myVar;
extern UpdateParameters myParam;
extern SPH sph;
extern Lighting lighting;

static Color unpackColor(uint32_t rgba) {
	Color color;
	color.r = static_cast<unsigned char>(rgba & 0xFF);
	color.g = static_cast<unsigned char>((rgba >> 8) & 0xFF);
	color.b = static_cast<unsigned char>((rgba >> 16) & 0xFF);
	color.a = static_cast<unsigned char>((rgba >> 24) & 0xFF);
	return color;
}

static uint32_t packColor(const Color& color) {
	return static_cast<uint32_t>(color.r)
		| (static_cast<uint32_t>(color.g) << 8)
		| (static_cast<uint32_t>(color.b) << 16)
		| (static_cast<uint32_t>(color.a) << 24);
}

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

EMSCRIPTEN_KEEPALIVE void web_set_threads_amount(int threads) {
	if (threads < 1) {
		threads = 1;
	}
	if (threads > 32) {
		threads = 32;
	}
	myVar.threadsAmount = threads;
}

EMSCRIPTEN_KEEPALIVE int web_get_threads_amount() {
	return myVar.threadsAmount;
}

EMSCRIPTEN_KEEPALIVE void web_set_domain_width(float width) {
	if (width < 200.0f) {
		width = 200.0f;
	}
	if (width > 3840.0f) {
		width = 3840.0f;
	}
	myVar.domainSize.x = width;
	myVar.halfDomainWidth = width * 0.5f;
}

EMSCRIPTEN_KEEPALIVE float web_get_domain_width() {
	return myVar.domainSize.x;
}

EMSCRIPTEN_KEEPALIVE void web_set_domain_height(float height) {
	if (height < 200.0f) {
		height = 200.0f;
	}
	if (height > 2160.0f) {
		height = 2160.0f;
	}
	myVar.domainSize.y = height;
	myVar.halfDomainHeight = height * 0.5f;
}

EMSCRIPTEN_KEEPALIVE float web_get_domain_height() {
	return myVar.domainSize.y;
}

EMSCRIPTEN_KEEPALIVE void web_set_black_hole_init_mass(float mult) {
	if (mult < 0.005f) {
		mult = 0.005f;
	}
	if (mult > 15.0f) {
		mult = 15.0f;
	}
	myParam.particlesSpawning.heavyParticleWeightMultiplier = mult;
}

EMSCRIPTEN_KEEPALIVE float web_get_black_hole_init_mass() {
	return myParam.particlesSpawning.heavyParticleWeightMultiplier;
}

EMSCRIPTEN_KEEPALIVE void web_set_ambient_temperature(float temp) {
	if (temp < 1.0f) {
		temp = 1.0f;
	}
	if (temp > 2500.0f) {
		temp = 2500.0f;
	}
	myVar.ambientTemp = temp;
}

EMSCRIPTEN_KEEPALIVE float web_get_ambient_temperature() {
	return myVar.ambientTemp;
}

EMSCRIPTEN_KEEPALIVE void web_set_ambient_heat_rate(float rate) {
	if (rate < 0.0f) {
		rate = 0.0f;
	}
	if (rate > 10.0f) {
		rate = 10.0f;
	}
	myVar.globalAmbientHeatRate = rate;
}

EMSCRIPTEN_KEEPALIVE float web_get_ambient_heat_rate() {
	return myVar.globalAmbientHeatRate;
}

EMSCRIPTEN_KEEPALIVE void web_set_heat_conductivity(float conductivity) {
	if (conductivity < 0.001f) {
		conductivity = 0.001f;
	}
	if (conductivity > 1.0f) {
		conductivity = 1.0f;
	}
	myVar.globalHeatConductivity = conductivity;
}

EMSCRIPTEN_KEEPALIVE float web_get_heat_conductivity() {
	return myVar.globalHeatConductivity;
}

EMSCRIPTEN_KEEPALIVE void web_set_constraint_stiffness(float mult) {
	if (mult < 0.001f) {
		mult = 0.001f;
	}
	if (mult > 3.0f) {
		mult = 3.0f;
	}
	myVar.globalConstraintStiffnessMult = mult;
}

EMSCRIPTEN_KEEPALIVE float web_get_constraint_stiffness() {
	return myVar.globalConstraintStiffnessMult;
}

EMSCRIPTEN_KEEPALIVE void web_set_constraint_resistance(float mult) {
	if (mult < 0.001f) {
		mult = 0.001f;
	}
	if (mult > 30.0f) {
		mult = 30.0f;
	}
	myVar.globalConstraintResistance = mult;
}

EMSCRIPTEN_KEEPALIVE float web_get_constraint_resistance() {
	return myVar.globalConstraintResistance;
}

EMSCRIPTEN_KEEPALIVE void web_set_fluid_vertical_gravity(float gravity) {
	if (gravity < 0.0f) {
		gravity = 0.0f;
	}
	if (gravity > 10.0f) {
		gravity = 10.0f;
	}
	sph.verticalGravity = gravity;
}

EMSCRIPTEN_KEEPALIVE float web_get_fluid_vertical_gravity() {
	return sph.verticalGravity;
}

EMSCRIPTEN_KEEPALIVE void web_set_fluid_mass_multiplier(float mass) {
	if (mass < 0.005f) {
		mass = 0.005f;
	}
	if (mass > 0.15f) {
		mass = 0.15f;
	}
	sph.mass = mass;
}

EMSCRIPTEN_KEEPALIVE float web_get_fluid_mass_multiplier() {
	return sph.mass;
}

EMSCRIPTEN_KEEPALIVE void web_set_fluid_viscosity(float viscosity) {
	if (viscosity < 0.01f) {
		viscosity = 0.01f;
	}
	if (viscosity > 15.0f) {
		viscosity = 15.0f;
	}
	sph.viscosity = viscosity;
}

EMSCRIPTEN_KEEPALIVE float web_get_fluid_viscosity() {
	return sph.viscosity;
}

EMSCRIPTEN_KEEPALIVE void web_set_fluid_stiffness(float stiffness) {
	if (stiffness < 0.01f) {
		stiffness = 0.01f;
	}
	if (stiffness > 15.0f) {
		stiffness = 15.0f;
	}
	sph.stiffMultiplier = stiffness;
}

EMSCRIPTEN_KEEPALIVE float web_get_fluid_stiffness() {
	return sph.stiffMultiplier;
}

EMSCRIPTEN_KEEPALIVE void web_set_fluid_cohesion(float cohesion) {
	if (cohesion < 0.0f) {
		cohesion = 0.0f;
	}
	if (cohesion > 10.0f) {
		cohesion = 10.0f;
	}
	sph.cohesionCoefficient = cohesion;
}

EMSCRIPTEN_KEEPALIVE float web_get_fluid_cohesion() {
	return sph.cohesionCoefficient;
}

EMSCRIPTEN_KEEPALIVE void web_set_fluid_delta(float delta) {
	if (delta < 500.0f) {
		delta = 500.0f;
	}
	if (delta > 20000.0f) {
		delta = 20000.0f;
	}
	sph.delta = delta;
}

EMSCRIPTEN_KEEPALIVE float web_get_fluid_delta() {
	return sph.delta;
}

EMSCRIPTEN_KEEPALIVE void web_set_fluid_max_velocity(float velocity) {
	if (velocity < 0.0f) {
		velocity = 0.0f;
	}
	if (velocity > 2000.0f) {
		velocity = 2000.0f;
	}
	myVar.sphMaxVel = velocity;
}

EMSCRIPTEN_KEEPALIVE float web_get_fluid_max_velocity() {
	return myVar.sphMaxVel;
}

EMSCRIPTEN_KEEPALIVE void web_set_optics_enabled(int enabled) {
	myVar.isOpticsEnabled = (enabled != 0);
	lighting.shouldRender = true;
}

EMSCRIPTEN_KEEPALIVE int web_get_optics_enabled() {
	return myVar.isOpticsEnabled ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_light_gain(float gain) {
	if (gain < 0.0f) {
		gain = 0.0f;
	}
	if (gain > 1.0f) {
		gain = 1.0f;
	}
	lighting.lightGain = gain;
	lighting.isSliderLightGain = true;
	lighting.shouldRender = true;
}

EMSCRIPTEN_KEEPALIVE float web_get_light_gain() {
	return lighting.lightGain;
}

EMSCRIPTEN_KEEPALIVE void web_set_light_spread(float spread) {
	if (spread < 0.0f) {
		spread = 0.0f;
	}
	if (spread > 1.0f) {
		spread = 1.0f;
	}
	lighting.lightSpread = spread;
	lighting.isSliderlightSpread = true;
	lighting.shouldRender = true;
}

EMSCRIPTEN_KEEPALIVE float web_get_light_spread() {
	return lighting.lightSpread;
}

EMSCRIPTEN_KEEPALIVE void web_set_wall_specular_roughness(float roughness) {
	if (roughness < 0.0f) {
		roughness = 0.0f;
	}
	if (roughness > 1.0f) {
		roughness = 1.0f;
	}
	lighting.wallSpecularRoughness = roughness;
	lighting.isSliderSpecularRough = true;
	lighting.shouldRender = true;
}

EMSCRIPTEN_KEEPALIVE float web_get_wall_specular_roughness() {
	return lighting.wallSpecularRoughness;
}

EMSCRIPTEN_KEEPALIVE void web_set_wall_refraction_roughness(float roughness) {
	if (roughness < 0.0f) {
		roughness = 0.0f;
	}
	if (roughness > 1.0f) {
		roughness = 1.0f;
	}
	lighting.wallRefractionRoughness = roughness;
	lighting.isSliderRefractionRough = true;
	lighting.shouldRender = true;
}

EMSCRIPTEN_KEEPALIVE float web_get_wall_refraction_roughness() {
	return lighting.wallRefractionRoughness;
}

EMSCRIPTEN_KEEPALIVE void web_set_wall_refraction_amount(float amount) {
	if (amount < 0.0f) {
		amount = 0.0f;
	}
	if (amount > 1.0f) {
		amount = 1.0f;
	}
	lighting.wallRefractionAmount = amount;
	lighting.isSliderRefractionAmount = true;
	lighting.shouldRender = true;
}

EMSCRIPTEN_KEEPALIVE float web_get_wall_refraction_amount() {
	return lighting.wallRefractionAmount;
}

EMSCRIPTEN_KEEPALIVE void web_set_wall_ior(float ior) {
	if (ior < 0.0f) {
		ior = 0.0f;
	}
	if (ior > 100.0f) {
		ior = 100.0f;
	}
	lighting.wallIOR = ior;
	lighting.isSliderIor = true;
	lighting.shouldRender = true;
}

EMSCRIPTEN_KEEPALIVE float web_get_wall_ior() {
	return lighting.wallIOR;
}

EMSCRIPTEN_KEEPALIVE void web_set_wall_dispersion(float dispersion) {
	if (dispersion < 0.0f) {
		dispersion = 0.0f;
	}
	if (dispersion > 0.2f) {
		dispersion = 0.2f;
	}
	lighting.wallDispersion = dispersion;
	lighting.isSliderDispersion = true;
	lighting.shouldRender = true;
}

EMSCRIPTEN_KEEPALIVE float web_get_wall_dispersion() {
	return lighting.wallDispersion;
}

EMSCRIPTEN_KEEPALIVE void web_set_wall_emission_gain(float gain) {
	if (gain < 0.0f) {
		gain = 0.0f;
	}
	if (gain > 1.0f) {
		gain = 1.0f;
	}
	lighting.wallEmissionGain = gain;
	lighting.isSliderEmissionGain = true;
	lighting.shouldRender = true;
}

EMSCRIPTEN_KEEPALIVE float web_get_wall_emission_gain() {
	return lighting.wallEmissionGain;
}

EMSCRIPTEN_KEEPALIVE void web_set_shape_relax_iter(int iter) {
	if (iter < 0) {
		iter = 0;
	}
	if (iter > 50) {
		iter = 50;
	}
	lighting.shapeRelaxIter = iter;
}

EMSCRIPTEN_KEEPALIVE int web_get_shape_relax_iter() {
	return lighting.shapeRelaxIter;
}

EMSCRIPTEN_KEEPALIVE void web_set_shape_relax_factor(float factor) {
	if (factor < 0.0f) {
		factor = 0.0f;
	}
	if (factor > 1.0f) {
		factor = 1.0f;
	}
	lighting.shapeRelaxFactor = factor;
}

EMSCRIPTEN_KEEPALIVE float web_get_shape_relax_factor() {
	return lighting.shapeRelaxFactor;
}

EMSCRIPTEN_KEEPALIVE void web_set_max_samples(int samples) {
	if (samples < 1) {
		samples = 1;
	}
	if (samples > 2048) {
		samples = 2048;
	}
	lighting.maxSamples = samples;
	lighting.shouldRender = true;
}

EMSCRIPTEN_KEEPALIVE int web_get_max_samples() {
	return lighting.maxSamples;
}

EMSCRIPTEN_KEEPALIVE void web_set_sample_rays_amount(int amount) {
	if (amount < 1) {
		amount = 1;
	}
	if (amount > 8192) {
		amount = 8192;
	}
	lighting.sampleRaysAmount = amount;
	lighting.shouldRender = true;
}

EMSCRIPTEN_KEEPALIVE int web_get_sample_rays_amount() {
	return lighting.sampleRaysAmount;
}

EMSCRIPTEN_KEEPALIVE void web_set_max_bounces(int bounces) {
	if (bounces < 0) {
		bounces = 0;
	}
	if (bounces > 16) {
		bounces = 16;
	}
	lighting.maxBounces = bounces;
	lighting.shouldRender = true;
}

EMSCRIPTEN_KEEPALIVE int web_get_max_bounces() {
	return lighting.maxBounces;
}

EMSCRIPTEN_KEEPALIVE void web_set_diffuse_enabled(int enabled) {
	lighting.isDiffuseEnabled = (enabled != 0);
	lighting.shouldRender = true;
}

EMSCRIPTEN_KEEPALIVE int web_get_diffuse_enabled() {
	return lighting.isDiffuseEnabled ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_specular_enabled(int enabled) {
	lighting.isSpecularEnabled = (enabled != 0);
	lighting.shouldRender = true;
}

EMSCRIPTEN_KEEPALIVE int web_get_specular_enabled() {
	return lighting.isSpecularEnabled ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_refraction_enabled(int enabled) {
	lighting.isRefractionEnabled = (enabled != 0);
	lighting.shouldRender = true;
}

EMSCRIPTEN_KEEPALIVE int web_get_refraction_enabled() {
	return lighting.isRefractionEnabled ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_dispersion_enabled(int enabled) {
	lighting.isDispersionEnabled = (enabled != 0);
	lighting.shouldRender = true;
}

EMSCRIPTEN_KEEPALIVE int web_get_dispersion_enabled() {
	return lighting.isDispersionEnabled ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_emission_enabled(int enabled) {
	lighting.isEmissionEnabled = (enabled != 0);
	lighting.shouldRender = true;
}

EMSCRIPTEN_KEEPALIVE int web_get_emission_enabled() {
	return lighting.isEmissionEnabled ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_symmetrical_lens(int enabled) {
	lighting.symmetricalLens = (enabled != 0);
}

EMSCRIPTEN_KEEPALIVE int web_get_symmetrical_lens() {
	return lighting.symmetricalLens ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_draw_normals(int enabled) {
	lighting.drawNormals = (enabled != 0);
}

EMSCRIPTEN_KEEPALIVE int web_get_draw_normals() {
	return lighting.drawNormals ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_relax_move(int enabled) {
	lighting.relaxMove = (enabled != 0);
}

EMSCRIPTEN_KEEPALIVE int web_get_relax_move() {
	return lighting.relaxMove ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_light_color(uint32_t rgba) {
	lighting.lightColor = unpackColor(rgba);
	lighting.isSliderLightColor = true;
	lighting.shouldRender = true;
}

EMSCRIPTEN_KEEPALIVE uint32_t web_get_light_color() {
	return packColor(lighting.lightColor);
}

EMSCRIPTEN_KEEPALIVE void web_set_wall_base_color(uint32_t rgba) {
	lighting.wallBaseColor = unpackColor(rgba);
	lighting.isSliderBaseColor = true;
	lighting.shouldRender = true;
}

EMSCRIPTEN_KEEPALIVE uint32_t web_get_wall_base_color() {
	return packColor(lighting.wallBaseColor);
}

EMSCRIPTEN_KEEPALIVE void web_set_wall_specular_color(uint32_t rgba) {
	lighting.wallSpecularColor = unpackColor(rgba);
	lighting.isSliderSpecularColor = true;
	lighting.shouldRender = true;
}

EMSCRIPTEN_KEEPALIVE uint32_t web_get_wall_specular_color() {
	return packColor(lighting.wallSpecularColor);
}

EMSCRIPTEN_KEEPALIVE void web_set_wall_refraction_color(uint32_t rgba) {
	lighting.wallRefractionColor = unpackColor(rgba);
	lighting.isSliderRefractionCol = true;
	lighting.shouldRender = true;
}

EMSCRIPTEN_KEEPALIVE uint32_t web_get_wall_refraction_color() {
	return packColor(lighting.wallRefractionColor);
}

EMSCRIPTEN_KEEPALIVE void web_set_wall_emission_color(uint32_t rgba) {
	lighting.wallEmissionColor = unpackColor(rgba);
	lighting.isSliderEmissionCol = true;
	lighting.shouldRender = true;
}

EMSCRIPTEN_KEEPALIVE uint32_t web_get_wall_emission_color() {
	return packColor(lighting.wallEmissionColor);
}

EMSCRIPTEN_KEEPALIVE int web_get_lighting_samples() {
	return lighting.currentSamples;
}

EMSCRIPTEN_KEEPALIVE void web_reset_lighting_samples() {
	lighting.currentSamples = 0;
	lighting.accumulatedRays = 0;
	lighting.shouldRender = true;
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
