#include "globalLogic.h"
#include "parameters.h"

#include <cstdint>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>
#include <emscripten/emscripten.h>

extern UpdateVariables myVar;
extern UpdateParameters myParam;
extern SPH sph;
extern Physics physics;
extern SaveSystem save;
extern Lighting lighting;
extern Field field;

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

static std::vector<uint8_t> g_sceneBuffer;
static std::string g_sceneJson;

struct SceneSnapshot {
	bool valid = false;
	float softening = 0.0f;
	float gravity = 0.0f;
	float theta = 0.0f;
	float timeStep = 0.0f;
	float timeFactor = 1.0f;
	int gravityRampEnabled = 0;
	float gravityRampStart = 0.0f;
	float gravityRampSeconds = 0.0f;
	int velocityDampingEnabled = 0;
	float velocityDampingRate = 0.0f;
	int darkMatterEnabled = 0;
	int gravityFieldEnabled = 0;
	float particleAmountMult = 0.0f;
	float darkMatterAmountMult = 0.0f;
	int massMultiplierEnabled = 0;
	float cameraTargetX = 0.0f;
	float cameraTargetY = 0.0f;
	float cameraOffsetX = 0.0f;
	float cameraOffsetY = 0.0f;
	float cameraZoom = 0.0f;
};

static SceneSnapshot g_lastSavedSnapshot;

static void appendVec2(std::ostringstream& out, const glm::vec2& value) {
	out << "[" << value.x << "," << value.y << "]";
}

static void appendVec2(std::ostringstream& out, const Vector2& value) {
	out << "[" << value.x << "," << value.y << "]";
}

static void appendColor(std::ostringstream& out, const Color& value) {
	out << "[" << static_cast<int>(value.r) << "," << static_cast<int>(value.g) << ","
		<< static_cast<int>(value.b) << "," << static_cast<int>(value.a) << "]";
}

static glm::vec2 readVec2(const YAML::Node& node) {
	if (!node || !node.IsSequence() || node.size() < 2) {
		return { 0.0f, 0.0f };
	}
	return { node[0].as<float>(), node[1].as<float>() };
}

static Vector2 readVector2(const YAML::Node& node) {
	if (!node || !node.IsSequence() || node.size() < 2) {
		return { 0.0f, 0.0f };
	}
	return { node[0].as<float>(), node[1].as<float>() };
}

static Color readColor(const YAML::Node& node, Color fallback) {
	Color result = fallback;
	if (!node || !node.IsSequence() || node.size() < 4) {
		return result;
	}
	auto clampChannel = [](int value) {
		if (value < 0) return 0;
		if (value > 255) return 255;
		return value;
	};
	result.r = static_cast<unsigned char>(clampChannel(node[0].as<int>()));
	result.g = static_cast<unsigned char>(clampChannel(node[1].as<int>()));
	result.b = static_cast<unsigned char>(clampChannel(node[2].as<int>()));
	result.a = static_cast<unsigned char>(clampChannel(node[3].as<int>()));
	return result;
}

static std::vector<glm::vec2> readVec2List(const YAML::Node& node) {
	std::vector<glm::vec2> values;
	if (!node || !node.IsSequence()) {
		return values;
	}
	values.reserve(node.size());
	for (const auto& entry : node) {
		values.push_back(readVec2(entry));
	}
	return values;
}

static std::vector<uint32_t> readUintList(const YAML::Node& node) {
	std::vector<uint32_t> values;
	if (!node || !node.IsSequence()) {
		return values;
	}
	values.reserve(node.size());
	for (const auto& entry : node) {
		values.push_back(entry.as<uint32_t>());
	}
	return values;
}

static std::string ensureSceneDir() {
	const std::string dir = "/scenes";
	std::error_code ec;
	std::filesystem::create_directory(dir, ec);
	return dir;
}

static void logSceneDiffLine(const std::string& message, const char* color) {
#if defined(EMSCRIPTEN)
	const char* safeColor = color ? color : "red";
	EM_ASM({
		const msg = UTF8ToString($0);
		const color = UTF8ToString($1);
		if (typeof console !== "undefined" && console.log) {
			console.log("%c" + msg, "color:" + color + ";font-weight:bold;");
		}
	}, message.c_str(), safeColor);
#else
	std::cout << message << std::endl;
#endif
}

static SceneSnapshot captureSceneSnapshot() {
	SceneSnapshot snapshot;
	snapshot.valid = true;
	snapshot.softening = myVar.softening;
	snapshot.gravity = myVar.gravityMultiplier;
	snapshot.theta = myVar.theta;
	snapshot.timeStep = myVar.timeStepMultiplier;
	snapshot.timeFactor = myVar.timeFactor;
	snapshot.gravityRampEnabled = myVar.gravityRampEnabled ? 1 : 0;
	snapshot.gravityRampStart = myVar.gravityRampStartMult;
	snapshot.gravityRampSeconds = myVar.gravityRampSeconds;
	snapshot.velocityDampingEnabled = myVar.velocityDampingEnabled ? 1 : 0;
	snapshot.velocityDampingRate = myVar.velocityDampingPerSecond;
	snapshot.darkMatterEnabled = myVar.isDarkMatterEnabled ? 1 : 0;
	snapshot.gravityFieldEnabled = myVar.isGravityFieldEnabled ? 1 : 0;
	snapshot.particleAmountMult = myParam.particlesSpawning.particleAmountMultiplier;
	snapshot.darkMatterAmountMult = myParam.particlesSpawning.DMAmountMultiplier;
	snapshot.massMultiplierEnabled = myParam.particlesSpawning.massMultiplierEnabled ? 1 : 0;
	snapshot.cameraTargetX = myParam.myCamera.camera.target.x;
	snapshot.cameraTargetY = myParam.myCamera.camera.target.y;
	snapshot.cameraOffsetX = myParam.myCamera.camera.offset.x;
	snapshot.cameraOffsetY = myParam.myCamera.camera.offset.y;
	snapshot.cameraZoom = myParam.myCamera.camera.zoom;
	return snapshot;
}

static SceneSnapshot captureDefaultSnapshot() {
	UpdateVariables defaultVar;
	UpdateParameters defaultParam;
	SceneSnapshot snapshot;
	snapshot.valid = true;
	snapshot.softening = defaultVar.softening;
	snapshot.gravity = defaultVar.gravityMultiplier;
	snapshot.theta = defaultVar.theta;
	snapshot.timeStep = defaultVar.timeStepMultiplier;
	snapshot.timeFactor = defaultVar.timeFactor;
	snapshot.gravityRampEnabled = defaultVar.gravityRampEnabled ? 1 : 0;
	snapshot.gravityRampStart = defaultVar.gravityRampStartMult;
	snapshot.gravityRampSeconds = defaultVar.gravityRampSeconds;
	snapshot.velocityDampingEnabled = defaultVar.velocityDampingEnabled ? 1 : 0;
	snapshot.velocityDampingRate = defaultVar.velocityDampingPerSecond;
	snapshot.darkMatterEnabled = defaultVar.isDarkMatterEnabled ? 1 : 0;
	snapshot.gravityFieldEnabled = defaultVar.isGravityFieldEnabled ? 1 : 0;
	snapshot.particleAmountMult = defaultParam.particlesSpawning.particleAmountMultiplier;
	snapshot.darkMatterAmountMult = defaultParam.particlesSpawning.DMAmountMultiplier;
	snapshot.massMultiplierEnabled = defaultParam.particlesSpawning.massMultiplierEnabled ? 1 : 0;
	snapshot.cameraTargetX = defaultParam.myCamera.camera.target.x;
	snapshot.cameraTargetY = defaultParam.myCamera.camera.target.y;
	snapshot.cameraOffsetX = defaultParam.myCamera.camera.offset.x;
	snapshot.cameraOffsetY = defaultParam.myCamera.camera.offset.y;
	snapshot.cameraZoom = defaultParam.myCamera.camera.zoom;
	return snapshot;
}

static void logSceneLoadSummary(const char* source) {
	const char* label = source ? source : "unknown";
	std::cout
		<< "[SceneLoad][" << label << "]"
		<< " softening=" << myVar.softening
		<< " gravity=" << myVar.gravityMultiplier
		<< " theta=" << myVar.theta
		<< " timeStep=" << myVar.timeStepMultiplier
		<< " timeFactor=" << myVar.timeFactor
		<< " gravityRampEnabled=" << myVar.gravityRampEnabled
		<< " gravityRampStart=" << myVar.gravityRampStartMult
		<< " gravityRampSeconds=" << myVar.gravityRampSeconds
		<< " velocityDampingEnabled=" << myVar.velocityDampingEnabled
		<< " velocityDampingRate=" << myVar.velocityDampingPerSecond
		<< " darkMatter=" << myVar.isDarkMatterEnabled
		<< " gravityField=" << myVar.isGravityFieldEnabled
		<< " particleAmountMult=" << myParam.particlesSpawning.particleAmountMultiplier
		<< " darkMatterAmountMult=" << myParam.particlesSpawning.DMAmountMultiplier
		<< " massMultiplier=" << myParam.particlesSpawning.massMultiplierEnabled
		<< " cameraTarget=(" << myParam.myCamera.camera.target.x << "," << myParam.myCamera.camera.target.y << ")"
		<< " cameraOffset=(" << myParam.myCamera.camera.offset.x << "," << myParam.myCamera.camera.offset.y << ")"
		<< " cameraZoom=" << myParam.myCamera.camera.zoom
		<< std::endl;
}

static void logSceneSaveSummary(const char* source) {
	const char* label = source ? source : "unknown";
	std::cout
		<< "[SceneSave][" << label << "]"
		<< " softening=" << myVar.softening
		<< " gravity=" << myVar.gravityMultiplier
		<< " theta=" << myVar.theta
		<< " timeStep=" << myVar.timeStepMultiplier
		<< " timeFactor=" << myVar.timeFactor
		<< " gravityRampEnabled=" << myVar.gravityRampEnabled
		<< " gravityRampStart=" << myVar.gravityRampStartMult
		<< " gravityRampSeconds=" << myVar.gravityRampSeconds
		<< " velocityDampingEnabled=" << myVar.velocityDampingEnabled
		<< " velocityDampingRate=" << myVar.velocityDampingPerSecond
		<< " darkMatter=" << myVar.isDarkMatterEnabled
		<< " gravityField=" << myVar.isGravityFieldEnabled
		<< " particleAmountMult=" << myParam.particlesSpawning.particleAmountMultiplier
		<< " darkMatterAmountMult=" << myParam.particlesSpawning.DMAmountMultiplier
		<< " massMultiplier=" << myParam.particlesSpawning.massMultiplierEnabled
		<< " cameraTarget=(" << myParam.myCamera.camera.target.x << "," << myParam.myCamera.camera.target.y << ")"
		<< " cameraOffset=(" << myParam.myCamera.camera.offset.x << "," << myParam.myCamera.camera.offset.y << ")"
		<< " cameraZoom=" << myParam.myCamera.camera.zoom
		<< std::endl;
}

static bool floatDiff(float a, float b, float tolerance = 1e-4f) {
	return std::fabs(a - b) > tolerance;
}

static bool doubleDiff(double a, double b, double tolerance = 1e-9) {
	return std::fabs(a - b) > tolerance;
}

static void resetCameraAfterLoad() {
	myParam.myCamera.isFollowing = false;
	myParam.myCamera.centerCamera = false;
	myParam.myCamera.cameraChangedThisFrame = true;
}

static void compareWithLastSaved(const char* source) {
	if (!g_lastSavedSnapshot.valid) {
		const char* label = source ? source : "unknown";
		const std::string message = "[SceneLoad][compare:" + std::string(label) + "] no saved snapshot available.";
		logSceneDiffLine(message, "#d97706");
		return;
	}

	const SceneSnapshot loaded = captureSceneSnapshot();
	const char* label = source ? source : "unknown";
	bool anyDiff = false;

	auto logFloatDiff = [&](const char* name, float loadedValue, float savedValue) {
		if (floatDiff(loadedValue, savedValue)) {
			anyDiff = true;
			std::ostringstream line;
			line << "[SceneLoad][compare:" << label << "] " << name
				<< " loaded=" << loadedValue << " saved=" << savedValue
				<< " delta=" << (loadedValue - savedValue);
			logSceneDiffLine(line.str(), "#dc2626");
		}
	};

	auto logIntDiff = [&](const char* name, int loadedValue, int savedValue) {
		if (loadedValue != savedValue) {
			anyDiff = true;
			std::ostringstream line;
			line << "[SceneLoad][compare:" << label << "] " << name
				<< " loaded=" << loadedValue << " saved=" << savedValue;
			logSceneDiffLine(line.str(), "#dc2626");
		}
	};

	logFloatDiff("softening", loaded.softening, g_lastSavedSnapshot.softening);
	logFloatDiff("gravity", loaded.gravity, g_lastSavedSnapshot.gravity);
	logFloatDiff("theta", loaded.theta, g_lastSavedSnapshot.theta);
	logFloatDiff("timeStep", loaded.timeStep, g_lastSavedSnapshot.timeStep);
	logFloatDiff("timeFactor", loaded.timeFactor, g_lastSavedSnapshot.timeFactor);
	logIntDiff("gravityRampEnabled", loaded.gravityRampEnabled, g_lastSavedSnapshot.gravityRampEnabled);
	logFloatDiff("gravityRampStart", loaded.gravityRampStart, g_lastSavedSnapshot.gravityRampStart);
	logFloatDiff("gravityRampSeconds", loaded.gravityRampSeconds, g_lastSavedSnapshot.gravityRampSeconds);
	logIntDiff("velocityDampingEnabled", loaded.velocityDampingEnabled, g_lastSavedSnapshot.velocityDampingEnabled);
	logFloatDiff("velocityDampingRate", loaded.velocityDampingRate, g_lastSavedSnapshot.velocityDampingRate);
	logIntDiff("darkMatter", loaded.darkMatterEnabled, g_lastSavedSnapshot.darkMatterEnabled);
	logIntDiff("gravityField", loaded.gravityFieldEnabled, g_lastSavedSnapshot.gravityFieldEnabled);
	logFloatDiff("particleAmountMult", loaded.particleAmountMult, g_lastSavedSnapshot.particleAmountMult);
	logFloatDiff("darkMatterAmountMult", loaded.darkMatterAmountMult, g_lastSavedSnapshot.darkMatterAmountMult);
	logIntDiff("massMultiplier", loaded.massMultiplierEnabled, g_lastSavedSnapshot.massMultiplierEnabled);
	logFloatDiff("cameraTargetX", loaded.cameraTargetX, g_lastSavedSnapshot.cameraTargetX);
	logFloatDiff("cameraTargetY", loaded.cameraTargetY, g_lastSavedSnapshot.cameraTargetY);
	logFloatDiff("cameraOffsetX", loaded.cameraOffsetX, g_lastSavedSnapshot.cameraOffsetX);
	logFloatDiff("cameraOffsetY", loaded.cameraOffsetY, g_lastSavedSnapshot.cameraOffsetY);
	logFloatDiff("cameraZoom", loaded.cameraZoom, g_lastSavedSnapshot.cameraZoom);

	if (!anyDiff) {
		const std::string message = "[SceneLoad][compare:" + std::string(label) + "] no differences detected.";
		logSceneDiffLine(message, "#16a34a");
	} else {
		const std::string message = "[SceneLoad][compare:" + std::string(label) + "] differences detected.";
		logSceneDiffLine(message, "#dc2626");
	}
}

static void compareWithDefaults(const char* source) {
	const SceneSnapshot defaults = captureDefaultSnapshot();
	const SceneSnapshot loaded = captureSceneSnapshot();
	const char* label = source ? source : "unknown";
	bool anyDiff = false;

	auto logFloatDiff = [&](const char* name, float loadedValue, float defaultValue) {
		if (floatDiff(loadedValue, defaultValue)) {
			anyDiff = true;
			std::ostringstream line;
			line << "[SceneLoad][compare-defaults:" << label << "] " << name
				<< " loaded=" << loadedValue << " default=" << defaultValue
				<< " delta=" << (loadedValue - defaultValue);
			logSceneDiffLine(line.str(), "#dc2626");
		}
	};

	auto logIntDiff = [&](const char* name, int loadedValue, int defaultValue) {
		if (loadedValue != defaultValue) {
			anyDiff = true;
			std::ostringstream line;
			line << "[SceneLoad][compare-defaults:" << label << "] " << name
				<< " loaded=" << loadedValue << " default=" << defaultValue;
			logSceneDiffLine(line.str(), "#dc2626");
		}
	};

	logFloatDiff("softening", loaded.softening, defaults.softening);
	logFloatDiff("gravity", loaded.gravity, defaults.gravity);
	logFloatDiff("theta", loaded.theta, defaults.theta);
	logFloatDiff("timeStep", loaded.timeStep, defaults.timeStep);
	logFloatDiff("timeFactor", loaded.timeFactor, defaults.timeFactor);
	logIntDiff("gravityRampEnabled", loaded.gravityRampEnabled, defaults.gravityRampEnabled);
	logFloatDiff("gravityRampStart", loaded.gravityRampStart, defaults.gravityRampStart);
	logFloatDiff("gravityRampSeconds", loaded.gravityRampSeconds, defaults.gravityRampSeconds);
	logIntDiff("velocityDampingEnabled", loaded.velocityDampingEnabled, defaults.velocityDampingEnabled);
	logFloatDiff("velocityDampingRate", loaded.velocityDampingRate, defaults.velocityDampingRate);
	logIntDiff("darkMatter", loaded.darkMatterEnabled, defaults.darkMatterEnabled);
	logIntDiff("gravityField", loaded.gravityFieldEnabled, defaults.gravityFieldEnabled);
	logFloatDiff("particleAmountMult", loaded.particleAmountMult, defaults.particleAmountMult);
	logFloatDiff("darkMatterAmountMult", loaded.darkMatterAmountMult, defaults.darkMatterAmountMult);
	logIntDiff("massMultiplier", loaded.massMultiplierEnabled, defaults.massMultiplierEnabled);
	logFloatDiff("cameraTargetX", loaded.cameraTargetX, defaults.cameraTargetX);
	logFloatDiff("cameraTargetY", loaded.cameraTargetY, defaults.cameraTargetY);
	logFloatDiff("cameraOffsetX", loaded.cameraOffsetX, defaults.cameraOffsetX);
	logFloatDiff("cameraOffsetY", loaded.cameraOffsetY, defaults.cameraOffsetY);
	logFloatDiff("cameraZoom", loaded.cameraZoom, defaults.cameraZoom);

	if (!anyDiff) {
		const std::string message = "[SceneLoad][compare-defaults:" + std::string(label) + "] no differences detected.";
		logSceneDiffLine(message, "#16a34a");
	} else {
		const std::string message = "[SceneLoad][compare-defaults:" + std::string(label) + "] differences detected.";
		logSceneDiffLine(message, "#dc2626");
	}
}

static void compareUpdateVariablesWithDefaults(const char* source) {
	UpdateVariables defaults;
	const UpdateVariables& loaded = myVar;
	const char* label = source ? source : "unknown";
	bool anyDiff = false;

	auto logFloat = [&](const char* name, float loadedValue, float defaultValue) {
		if (floatDiff(loadedValue, defaultValue)) {
			anyDiff = true;
			std::ostringstream line;
			line << "[SceneLoad][compare-myVar:" << label << "] " << name
				<< " loaded=" << loadedValue << " default=" << defaultValue
				<< " delta=" << (loadedValue - defaultValue);
			logSceneDiffLine(line.str(), "#dc2626");
		}
	};

	auto logDouble = [&](const char* name, double loadedValue, double defaultValue) {
		if (doubleDiff(loadedValue, defaultValue)) {
			anyDiff = true;
			std::ostringstream line;
			line << "[SceneLoad][compare-myVar:" << label << "] " << name
				<< " loaded=" << loadedValue << " default=" << defaultValue
				<< " delta=" << (loadedValue - defaultValue);
			logSceneDiffLine(line.str(), "#dc2626");
		}
	};

	auto logInt = [&](const char* name, int loadedValue, int defaultValue) {
		if (loadedValue != defaultValue) {
			anyDiff = true;
			std::ostringstream line;
			line << "[SceneLoad][compare-myVar:" << label << "] " << name
				<< " loaded=" << loadedValue << " default=" << defaultValue;
			logSceneDiffLine(line.str(), "#dc2626");
		}
	};

	auto logBool = [&](const char* name, bool loadedValue, bool defaultValue) {
		if (loadedValue != defaultValue) {
			anyDiff = true;
			std::ostringstream line;
			line << "[SceneLoad][compare-myVar:" << label << "] " << name
				<< " loaded=" << (loadedValue ? 1 : 0) << " default=" << (defaultValue ? 1 : 0);
			logSceneDiffLine(line.str(), "#dc2626");
		}
	};

	auto logVec2 = [&](const char* name, const glm::vec2& loadedValue, const glm::vec2& defaultValue) {
		if (floatDiff(loadedValue.x, defaultValue.x) || floatDiff(loadedValue.y, defaultValue.y)) {
			anyDiff = true;
			std::ostringstream line;
			line << "[SceneLoad][compare-myVar:" << label << "] " << name
				<< " loaded=(" << loadedValue.x << "," << loadedValue.y << ")"
				<< " default=(" << defaultValue.x << "," << defaultValue.y << ")";
			logSceneDiffLine(line.str(), "#dc2626");
		}
	};

	logInt("screenWidth", loaded.screenWidth, defaults.screenWidth);
	logInt("screenHeight", loaded.screenHeight, defaults.screenHeight);
	logFloat("halfScreenWidth", loaded.halfScreenWidth, defaults.halfScreenWidth);
	logFloat("halfScreenHeight", loaded.halfScreenHeight, defaults.halfScreenHeight);
	logFloat("screenRatioX", loaded.screenRatioX, defaults.screenRatioX);
	logFloat("screenRatioY", loaded.screenRatioY, defaults.screenRatioY);
	logVec2("domainSize", loaded.domainSize, defaults.domainSize);
	logFloat("halfDomainWidth", loaded.halfDomainWidth, defaults.halfDomainWidth);
	logFloat("halfDomainHeight", loaded.halfDomainHeight, defaults.halfDomainHeight);
	logBool("fullscreenState", loaded.fullscreenState, defaults.fullscreenState);
	logBool("exitGame", loaded.exitGame, defaults.exitGame);
	logInt("targetFPS", loaded.targetFPS, defaults.targetFPS);
	logDouble("G", loaded.G, defaults.G);
	logFloat("gravityMultiplier", loaded.gravityMultiplier, defaults.gravityMultiplier);
	logBool("gravityRampEnabled", loaded.gravityRampEnabled, defaults.gravityRampEnabled);
	logFloat("gravityRampStartMult", loaded.gravityRampStartMult, defaults.gravityRampStartMult);
	logFloat("gravityRampSeconds", loaded.gravityRampSeconds, defaults.gravityRampSeconds);
	logFloat("gravityRampTime", loaded.gravityRampTime, defaults.gravityRampTime);
	logFloat("softening", loaded.softening, defaults.softening);
	logFloat("theta", loaded.theta, defaults.theta);
	logFloat("timeStepMultiplier", loaded.timeStepMultiplier, defaults.timeStepMultiplier);
	logBool("useSymplecticIntegrator", loaded.useSymplecticIntegrator, defaults.useSymplecticIntegrator);
	logFloat("sphMaxVel", loaded.sphMaxVel, defaults.sphMaxVel);
	logFloat("globalHeatConductivity", loaded.globalHeatConductivity, defaults.globalHeatConductivity);
	logFloat("globalAmbientHeatRate", loaded.globalAmbientHeatRate, defaults.globalAmbientHeatRate);
	logFloat("ambientTemp", loaded.ambientTemp, defaults.ambientTemp);
	logInt("maxLeafParticles", loaded.maxLeafParticles, defaults.maxLeafParticles);
	logFloat("minLeafSize", loaded.minLeafSize, defaults.minLeafSize);
	logFloat("fixedDeltaTime", loaded.fixedDeltaTime, defaults.fixedDeltaTime);
	logBool("isTimePlaying", loaded.isTimePlaying, defaults.isTimePlaying);
	logFloat("timeFactor", loaded.timeFactor, defaults.timeFactor);
	logBool("velocityDampingEnabled", loaded.velocityDampingEnabled, defaults.velocityDampingEnabled);
	logFloat("velocityDampingPerSecond", loaded.velocityDampingPerSecond, defaults.velocityDampingPerSecond);
	logBool("isGlobalTrailsEnabled", loaded.isGlobalTrailsEnabled, defaults.isGlobalTrailsEnabled);
	logBool("isSelectedTrailsEnabled", loaded.isSelectedTrailsEnabled, defaults.isSelectedTrailsEnabled);
	logBool("isLocalTrailsEnabled", loaded.isLocalTrailsEnabled, defaults.isLocalTrailsEnabled);
	logBool("isPeriodicBoundaryEnabled", loaded.isPeriodicBoundaryEnabled, defaults.isPeriodicBoundaryEnabled);
	logBool("isMultiThreadingEnabled", loaded.isMultiThreadingEnabled, defaults.isMultiThreadingEnabled);
	logBool("isBarnesHutEnabled", loaded.isBarnesHutEnabled, defaults.isBarnesHutEnabled);
	logBool("isDarkMatterEnabled", loaded.isDarkMatterEnabled, defaults.isDarkMatterEnabled);
	logBool("isDensitySizeEnabled", loaded.isDensitySizeEnabled, defaults.isDensitySizeEnabled);
	logBool("isForceSizeEnabled", loaded.isForceSizeEnabled, defaults.isForceSizeEnabled);
	logBool("isShipGasEnabled", loaded.isShipGasEnabled, defaults.isShipGasEnabled);
	logBool("isSPHEnabled", loaded.isSPHEnabled, defaults.isSPHEnabled);
	logBool("sphGround", loaded.sphGround, defaults.sphGround);
	logBool("isTempEnabled", loaded.isTempEnabled, defaults.isTempEnabled);
	logBool("constraintsEnabled", loaded.constraintsEnabled, defaults.constraintsEnabled);
	logBool("isOpticsEnabled", loaded.isOpticsEnabled, defaults.isOpticsEnabled);
	logBool("isGPUEnabled", loaded.isGPUEnabled, defaults.isGPUEnabled);
	logBool("isMergerEnabled", loaded.isMergerEnabled, defaults.isMergerEnabled);
	logBool("longExposureFlag", loaded.longExposureFlag, defaults.longExposureFlag);
	logInt("longExposureDuration", loaded.longExposureDuration, defaults.longExposureDuration);
	logInt("longExposureCurrent", loaded.longExposureCurrent, defaults.longExposureCurrent);
	logBool("isSpawningAllowed", loaded.isSpawningAllowed, defaults.isSpawningAllowed);
	logFloat("particleTextureHalfSize", loaded.particleTextureHalfSize, defaults.particleTextureHalfSize);
	logInt("trailMaxLength", loaded.trailMaxLength, defaults.trailMaxLength);
	logBool("isRecording", loaded.isRecording, defaults.isRecording);
	logFloat("particleSizeMultiplier", loaded.particleSizeMultiplier, defaults.particleSizeMultiplier);
	logBool("isDragging", loaded.isDragging, defaults.isDragging);
	logBool("isMouseNotHoveringUI", loaded.isMouseNotHoveringUI, defaults.isMouseNotHoveringUI);
	logBool("drawQuadtree", loaded.drawQuadtree, defaults.drawQuadtree);
	logBool("drawZCurves", loaded.drawZCurves, defaults.drawZCurves);
	logBool("isGlowEnabled", loaded.isGlowEnabled, defaults.isGlowEnabled);
	logVec2("mouseWorldPos", loaded.mouseWorldPos, defaults.mouseWorldPos);
	logInt("threadsAmount", loaded.threadsAmount, defaults.threadsAmount);
	logBool("pauseAfterRecording", loaded.pauseAfterRecording, defaults.pauseAfterRecording);
	logBool("cleanSceneAfterRecording", loaded.cleanSceneAfterRecording, defaults.cleanSceneAfterRecording);
	logFloat("recordingTimeLimit", loaded.recordingTimeLimit, defaults.recordingTimeLimit);
	logFloat("globalConstraintStiffnessMult", loaded.globalConstraintStiffnessMult, defaults.globalConstraintStiffnessMult);
	logFloat("globalConstraintResistance", loaded.globalConstraintResistance, defaults.globalConstraintResistance);
	logBool("constraintAllSolids", loaded.constraintAllSolids, defaults.constraintAllSolids);
	logBool("constraintSelected", loaded.constraintSelected, defaults.constraintSelected);
	logBool("deleteAllConstraints", loaded.deleteAllConstraints, defaults.deleteAllConstraints);
	logBool("deleteSelectedConstraints", loaded.deleteSelectedConstraints, defaults.deleteSelectedConstraints);
	logBool("drawConstraints", loaded.drawConstraints, defaults.drawConstraints);
	logBool("visualizeMesh", loaded.visualizeMesh, defaults.visualizeMesh);
	logBool("unbreakableConstraints", loaded.unbreakableConstraints, defaults.unbreakableConstraints);
	logBool("constraintStressColor", loaded.constraintStressColor, defaults.constraintStressColor);
	logBool("constraintAfterDrawingFlag", loaded.constraintAfterDrawingFlag, defaults.constraintAfterDrawingFlag);
	logBool("constraintAfterDrawing", loaded.constraintAfterDrawing, defaults.constraintAfterDrawing);
	logFloat("constraintMaxStressColor", loaded.constraintMaxStressColor, defaults.constraintMaxStressColor);
	logBool("pinFlag", loaded.pinFlag, defaults.pinFlag);
	logBool("unPinFlag", loaded.unPinFlag, defaults.unPinFlag);
	logBool("isBrushDrawing", loaded.isBrushDrawing, defaults.isBrushDrawing);
	logBool("autoPausedForBrush", loaded.autoPausedForBrush, defaults.autoPausedForBrush);
	logBool("wasTimePlayingBeforeBrush", loaded.wasTimePlayingBeforeBrush, defaults.wasTimePlayingBeforeBrush);
	logBool("showBrushCursor", loaded.showBrushCursor, defaults.showBrushCursor);
	logInt("introFontSize", loaded.introFontSize, defaults.introFontSize);
	logBool("gridExists", loaded.gridExists, defaults.gridExists);
	logBool("loadDropDownMenus", loaded.loadDropDownMenus, defaults.loadDropDownMenus);
	logBool("exportPlyFlag", loaded.exportPlyFlag, defaults.exportPlyFlag);
	logBool("exportPlySeqFlag", loaded.exportPlySeqFlag, defaults.exportPlySeqFlag);
	logInt("plyFrameNumber", loaded.plyFrameNumber, defaults.plyFrameNumber);
	logBool("toolSpawnHeavyParticle", loaded.toolSpawnHeavyParticle, defaults.toolSpawnHeavyParticle);
	logBool("toolDrawParticles", loaded.toolDrawParticles, defaults.toolDrawParticles);
	logBool("toolSpawnSmallGalaxy", loaded.toolSpawnSmallGalaxy, defaults.toolSpawnSmallGalaxy);
	logBool("toolSpawnBigGalaxy", loaded.toolSpawnBigGalaxy, defaults.toolSpawnBigGalaxy);
	logBool("toolSpawnStar", loaded.toolSpawnStar, defaults.toolSpawnStar);
	logBool("toolSpawnBigBang", loaded.toolSpawnBigBang, defaults.toolSpawnBigBang);
	logBool("toolErase", loaded.toolErase, defaults.toolErase);
	logBool("toolRadialForce", loaded.toolRadialForce, defaults.toolRadialForce);
	logBool("toolSpin", loaded.toolSpin, defaults.toolSpin);
	logBool("toolMove", loaded.toolMove, defaults.toolMove);
	logBool("toolRaiseTemp", loaded.toolRaiseTemp, defaults.toolRaiseTemp);
	logBool("toolLowerTemp", loaded.toolLowerTemp, defaults.toolLowerTemp);
	logBool("toolPointLight", loaded.toolPointLight, defaults.toolPointLight);
	logBool("toolAreaLight", loaded.toolAreaLight, defaults.toolAreaLight);
	logBool("toolConeLight", loaded.toolConeLight, defaults.toolConeLight);
	logBool("toolCircle", loaded.toolCircle, defaults.toolCircle);
	logBool("toolDrawShape", loaded.toolDrawShape, defaults.toolDrawShape);
	logBool("toolLens", loaded.toolLens, defaults.toolLens);
	logBool("toolWall", loaded.toolWall, defaults.toolWall);
	logBool("toolMoveOptics", loaded.toolMoveOptics, defaults.toolMoveOptics);
	logBool("toolEraseOptics", loaded.toolEraseOptics, defaults.toolEraseOptics);
	logBool("toolSelectOptics", loaded.toolSelectOptics, defaults.toolSelectOptics);
	logBool("isGravityFieldEnabled", loaded.isGravityFieldEnabled, defaults.isGravityFieldEnabled);
	logBool("gravityFieldDMParticles", loaded.gravityFieldDMParticles, defaults.gravityFieldDMParticles);

	if (!anyDiff) {
		const std::string message = "[SceneLoad][compare-myVar:" + std::string(label) + "] no differences detected.";
		logSceneDiffLine(message, "#16a34a");
	} else {
		const std::string message = "[SceneLoad][compare-myVar:" + std::string(label) + "] differences detected.";
		logSceneDiffLine(message, "#dc2626");
	}
}

static void notifyWebUiSync() {
	EM_ASM({
		if (typeof window !== "undefined" && window.webUiSyncFromEngine) {
			window.webUiSyncFromEngine();
		}
	});
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

EMSCRIPTEN_KEEPALIVE void web_set_gravity_ramp_enabled(int enabled) {
	myVar.gravityRampEnabled = (enabled != 0);
	if (myVar.gravityRampEnabled) {
		myVar.gravityRampTime = 0.0f;
	}
}

EMSCRIPTEN_KEEPALIVE int web_get_gravity_ramp_enabled() {
	return myVar.gravityRampEnabled ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_gravity_ramp_start_mult(float mult) {
	myVar.gravityRampStartMult = mult;
}

EMSCRIPTEN_KEEPALIVE float web_get_gravity_ramp_start_mult() {
	return myVar.gravityRampStartMult;
}

EMSCRIPTEN_KEEPALIVE void web_set_gravity_ramp_seconds(float seconds) {
	if (seconds < 0.0f) {
		seconds = 0.0f;
	}
	myVar.gravityRampSeconds = seconds;
	if (myVar.gravityRampSeconds > 0.0f) {
		myVar.gravityRampTime = std::min(myVar.gravityRampTime, myVar.gravityRampSeconds);
	}
}

EMSCRIPTEN_KEEPALIVE float web_get_gravity_ramp_seconds() {
	return myVar.gravityRampSeconds;
}

EMSCRIPTEN_KEEPALIVE void web_set_gravity_ramp_time(float time) {
	if (time < 0.0f) {
		time = 0.0f;
	}
	if (myVar.gravityRampSeconds > 0.0f) {
		time = std::min(time, myVar.gravityRampSeconds);
	}
	myVar.gravityRampTime = time;
}

EMSCRIPTEN_KEEPALIVE float web_get_gravity_ramp_time() {
	return myVar.gravityRampTime;
}

EMSCRIPTEN_KEEPALIVE void web_set_velocity_damping_enabled(int enabled) {
	myVar.velocityDampingEnabled = (enabled != 0);
}

EMSCRIPTEN_KEEPALIVE int web_get_velocity_damping_enabled() {
	return myVar.velocityDampingEnabled ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_velocity_damping_rate(float rate) {
	if (rate < 0.0f) {
		rate = 0.0f;
	}
	myVar.velocityDampingPerSecond = rate;
}

EMSCRIPTEN_KEEPALIVE float web_get_velocity_damping_rate() {
	return myVar.velocityDampingPerSecond;
}

EMSCRIPTEN_KEEPALIVE void web_set_time_step_multiplier(float mult) {
	myVar.timeStepMultiplier = mult;
}

EMSCRIPTEN_KEEPALIVE float web_get_time_step_multiplier() {
	return myVar.timeStepMultiplier;
}

EMSCRIPTEN_KEEPALIVE void web_set_symplectic_integrator(int enabled) {
	myVar.useSymplecticIntegrator = (enabled != 0);
}

EMSCRIPTEN_KEEPALIVE int web_get_symplectic_integrator() {
	return myVar.useSymplecticIntegrator ? 1 : 0;
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

	lighting.rays.clear();
	lighting.pointLights.clear();
	lighting.areaLights.clear();
	lighting.coneLights.clear();
	lighting.walls.clear();
	lighting.shapes.clear();
	lighting.wallPointers.clear();
	lighting.bvh.build(lighting.wallPointers);
	lighting.currentSamples = 0;
	lighting.accumulatedRays = 0;
	lighting.totalLights = 0;
	lighting.shouldRender = true;
}

EMSCRIPTEN_KEEPALIVE int web_save_scene(const char* path) {
	save.saveFlag = false;
	save.loadFlag = false;
	if (!path || !path[0]) {
		return 0;
	}

	logSceneSaveSummary("file");
	g_lastSavedSnapshot = captureSceneSnapshot();

	save.saveFlag = true;
	save.saveSystem(path, myVar, myParam, sph, physics, lighting, field);
	save.saveFlag = false;

	std::ifstream file(path, std::ios::binary);
	return file.good() ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE int web_load_scene(const char* path) {
	save.saveFlag = false;
	save.loadFlag = false;
	if (!path || !path[0]) {
		return 0;
	}

	std::ifstream file(path, std::ios::binary);
	if (!file.good()) {
		return 0;
	}
	file.close();

	save.loadFlag = true;
	save.saveSystem(path, myVar, myParam, sph, physics, lighting, field);
	save.loadFlag = false;
	resetCameraAfterLoad();
	logSceneLoadSummary("file");
	compareWithLastSaved("file");
	compareWithDefaults("file");
	compareUpdateVariablesWithDefaults("file");
	notifyWebUiSync();
	return 1;
}

EMSCRIPTEN_KEEPALIVE int web_save_scene_to_buffer() {
	save.saveFlag = false;
	save.loadFlag = false;
	const std::string dir = ensureSceneDir();
	const std::string path = dir + "/scene.bin";

	logSceneSaveSummary("buffer");
	g_lastSavedSnapshot = captureSceneSnapshot();

	save.saveFlag = true;
	save.saveSystem(path, myVar, myParam, sph, physics, lighting, field);
	save.saveFlag = false;

	std::ifstream file(path, std::ios::binary);
	if (!file.is_open()) {
		g_sceneBuffer.clear();
		return 0;
	}
	file.seekg(0, std::ios::end);
	std::streamsize size = file.tellg();
	if (size <= 0) {
		g_sceneBuffer.clear();
		return 0;
	}
	file.seekg(0, std::ios::beg);
	g_sceneBuffer.resize(static_cast<size_t>(size));
	file.read(reinterpret_cast<char*>(g_sceneBuffer.data()), size);
	file.close();
	std::filesystem::remove(path);
	return static_cast<int>(g_sceneBuffer.size());
}

EMSCRIPTEN_KEEPALIVE void web_capture_scene_snapshot() {
	logSceneSaveSummary("ui");
	g_lastSavedSnapshot = captureSceneSnapshot();
}

EMSCRIPTEN_KEEPALIVE uint8_t* web_get_scene_buffer_ptr() {
	if (g_sceneBuffer.empty()) {
		return nullptr;
	}
	return g_sceneBuffer.data();
}

EMSCRIPTEN_KEEPALIVE int web_load_scene_from_buffer(const uint8_t* data, int size) {
	if (!data || size <= 0) {
		return 0;
	}
	const std::string dir = ensureSceneDir();
	const std::string path = dir + "/scene.bin";
	std::ofstream file(path, std::ios::binary);
	if (!file.is_open()) {
		return 0;
	}
	file.write(reinterpret_cast<const char*>(data), size);
	file.close();

	save.saveFlag = false;
	save.loadFlag = true;
	save.saveSystem(path, myVar, myParam, sph, physics, lighting, field);
	save.loadFlag = false;
	std::filesystem::remove(path);
	resetCameraAfterLoad();
	logSceneLoadSummary("buffer");
	compareWithLastSaved("buffer");
	compareWithDefaults("buffer");
	compareUpdateVariablesWithDefaults("buffer");
	notifyWebUiSync();
	return 1;
}

EMSCRIPTEN_KEEPALIVE int web_build_scene_json() {
	std::ostringstream out;
	out << "{\"version\":1,";
	out << "\"camera\":{";
	out << "\"target\":";
	appendVec2(out, myParam.myCamera.camera.target);
	out << ",\"offset\":";
	appendVec2(out, myParam.myCamera.camera.offset);
	out << ",\"zoom\":" << myParam.myCamera.camera.zoom;
	out << ",\"rotation\":" << myParam.myCamera.camera.rotation;
	out << "},";

	out << "\"walls\":[";
	bool first = true;
	for (const Wall& w : lighting.walls) {
		if (!first) {
			out << ",";
		}
		first = false;
		out << "{";
		out << "\"vA\":";
		appendVec2(out, w.vA);
		out << ",\"vB\":";
		appendVec2(out, w.vB);
		out << ",\"normal\":";
		appendVec2(out, w.normal);
		out << ",\"normalVA\":";
		appendVec2(out, w.normalVA);
		out << ",\"normalVB\":";
		appendVec2(out, w.normalVB);
		out << ",\"isBeingSpawned\":" << (w.isBeingSpawned ? "true" : "false");
		out << ",\"vAisBeingMoved\":" << (w.vAisBeingMoved ? "true" : "false");
		out << ",\"vBisBeingMoved\":" << (w.vBisBeingMoved ? "true" : "false");
		out << ",\"apparentColor\":";
		appendColor(out, w.apparentColor);
		out << ",\"baseColor\":";
		appendColor(out, w.baseColor);
		out << ",\"specularColor\":";
		appendColor(out, w.specularColor);
		out << ",\"refractionColor\":";
		appendColor(out, w.refractionColor);
		out << ",\"emissionColor\":";
		appendColor(out, w.emissionColor);
		out << ",\"baseColorVal\":" << w.baseColorVal;
		out << ",\"specularColorVal\":" << w.specularColorVal;
		out << ",\"refractionColorVal\":" << w.refractionColorVal;
		out << ",\"specularRoughness\":" << w.specularRoughness;
		out << ",\"refractionRoughness\":" << w.refractionRoughness;
		out << ",\"refractionAmount\":" << w.refractionAmount;
		out << ",\"ior\":" << w.IOR;
		out << ",\"dispersion\":" << w.dispersionStrength;
		out << ",\"isShapeWall\":" << (w.isShapeWall ? "true" : "false");
		out << ",\"isShapeClosed\":" << (w.isShapeClosed ? "true" : "false");
		out << ",\"shapeId\":" << w.shapeId;
		out << ",\"id\":" << w.id;
		out << ",\"isSelected\":" << (w.isSelected ? "true" : "false");
		out << "}";
	}
	out << "],";

	out << "\"shapes\":[";
	first = true;
	for (const Shape& s : lighting.shapes) {
		if (!first) {
			out << ",";
		}
		first = false;
		out << "{";
		out << "\"wallIds\":[";
		for (size_t i = 0; i < s.myWallIds.size(); ++i) {
			if (i) out << ",";
			out << s.myWallIds[i];
		}
		out << "]";
		std::vector<glm::vec2> polygon = s.polygonVerts;
		if (polygon.empty() && !s.myWallIds.empty()) {
			for (uint32_t wallId : s.myWallIds) {
				const Wall* w = s.getWallById(lighting.walls, wallId);
				if (w) {
					polygon.push_back(w->vA);
				}
			}
		}
		out << ",\"polygon\":[";
		for (size_t i = 0; i < polygon.size(); ++i) {
			if (i) out << ",";
			appendVec2(out, polygon[i]);
		}
		out << "]";
		out << ",\"helpers\":[";
		for (size_t i = 0; i < s.helpers.size(); ++i) {
			if (i) out << ",";
			appendVec2(out, s.helpers[i]);
		}
		out << "]";
		out << ",\"baseColor\":";
		appendColor(out, s.baseColor);
		out << ",\"specularColor\":";
		appendColor(out, s.specularColor);
		out << ",\"refractionColor\":";
		appendColor(out, s.refractionColor);
		out << ",\"emissionColor\":";
		appendColor(out, s.emissionColor);
		out << ",\"specularRoughness\":" << s.specularRoughness;
		out << ",\"refractionRoughness\":" << s.refractionRoughness;
		out << ",\"refractionAmount\":" << s.refractionAmount;
		out << ",\"ior\":" << s.IOR;
		out << ",\"dispersion\":" << s.dispersionStrength;
		out << ",\"id\":" << s.id;
		out << ",\"h1\":";
		appendVec2(out, s.h1);
		out << ",\"h2\":";
		appendVec2(out, s.h2);
		out << ",\"isBeingSpawned\":" << (s.isBeingSpawned ? "true" : "false");
		out << ",\"isBeingMoved\":" << (s.isBeingMoved ? "true" : "false");
		out << ",\"isShapeClosed\":" << (s.isShapeClosed ? "true" : "false");
		out << ",\"type\":" << static_cast<int>(s.shapeType);
		out << ",\"drawHoverHelpers\":" << (s.drawHoverHelpers ? "true" : "false");
		out << ",\"oldDrawHelperPos\":";
		appendVec2(out, s.oldDrawHelperPos);
		out << ",\"secondHelper\":" << (s.secondHelper ? "true" : "false");
		out << ",\"thirdHelper\":" << (s.thirdHelper ? "true" : "false");
		out << ",\"fourthHelper\":" << (s.fourthHelper ? "true" : "false");
		out << ",\"Tempsh2Length\":" << s.Tempsh2Length;
		out << ",\"Tempsh2LengthSymmetry\":" << s.Tempsh2LengthSymmetry;
		out << ",\"tempDist\":" << s.tempDist;
		out << ",\"moveH2\":";
		appendVec2(out, s.moveH2);
		out << ",\"isThirdBeingMoved\":" << (s.isThirdBeingMoved ? "true" : "false");
		out << ",\"isFourthBeingMoved\":" << (s.isFourthBeingMoved ? "true" : "false");
		out << ",\"isFifthBeingMoved\":" << (s.isFifthBeingMoved ? "true" : "false");
		out << ",\"isFifthFourthMoved\":" << (s.isFifthFourthMoved ? "true" : "false");
		out << ",\"symmetricalLens\":" << (s.symmetricalLens ? "true" : "false");
		out << ",\"wallAId\":" << s.wallAId;
		out << ",\"wallBId\":" << s.wallBId;
		out << ",\"wallCId\":" << s.wallCId;
		out << ",\"lensSegments\":" << s.lensSegments;
		out << ",\"startAngle\":" << s.startAngle;
		out << ",\"endAngle\":" << s.endAngle;
		out << ",\"startAngleSymmetry\":" << s.startAngleSymmetry;
		out << ",\"endAngleSymmetry\":" << s.endAngleSymmetry;
		out << ",\"center\":";
		appendVec2(out, s.center);
		out << ",\"radius\":" << s.radius;
		out << ",\"centerSymmetry\":";
		appendVec2(out, s.centerSymmetry);
		out << ",\"radiusSymmetry\":" << s.radiusSymmetry;
		out << ",\"arcEnd\":";
		appendVec2(out, s.arcEnd);
		out << ",\"globalLensPrev\":";
		appendVec2(out, s.globalLensPrev);
		out << "}";
	}
	out << "],";

	out << "\"lights\":{";
	out << "\"point\":[";
	first = true;
	for (const PointLight& pl : lighting.pointLights) {
		if (!first) {
			out << ",";
		}
		first = false;
		out << "{";
		out << "\"pos\":";
		appendVec2(out, pl.pos);
		out << ",\"isBeingMoved\":" << (pl.isBeingMoved ? "true" : "false");
		out << ",\"color\":";
		appendColor(out, pl.color);
		out << ",\"apparentColor\":";
		appendColor(out, pl.apparentColor);
		out << ",\"isSelected\":" << (pl.isSelected ? "true" : "false");
		out << "}";
	}
	out << "],";
	out << "\"area\":[";
	first = true;
	for (const AreaLight& al : lighting.areaLights) {
		if (!first) {
			out << ",";
		}
		first = false;
		out << "{";
		out << "\"vA\":";
		appendVec2(out, al.vA);
		out << ",\"vB\":";
		appendVec2(out, al.vB);
		out << ",\"isBeingSpawned\":" << (al.isBeingSpawned ? "true" : "false");
		out << ",\"vAisBeingMoved\":" << (al.vAisBeingMoved ? "true" : "false");
		out << ",\"vBisBeingMoved\":" << (al.vBisBeingMoved ? "true" : "false");
		out << ",\"color\":";
		appendColor(out, al.color);
		out << ",\"apparentColor\":";
		appendColor(out, al.apparentColor);
		out << ",\"isSelected\":" << (al.isSelected ? "true" : "false");
		out << ",\"spread\":" << al.spread;
		out << "}";
	}
	out << "],";
	out << "\"cone\":[";
	first = true;
	for (const ConeLight& cl : lighting.coneLights) {
		if (!first) {
			out << ",";
		}
		first = false;
		out << "{";
		out << "\"vA\":";
		appendVec2(out, cl.vA);
		out << ",\"vB\":";
		appendVec2(out, cl.vB);
		out << ",\"isBeingSpawned\":" << (cl.isBeingSpawned ? "true" : "false");
		out << ",\"vAisBeingMoved\":" << (cl.vAisBeingMoved ? "true" : "false");
		out << ",\"vBisBeingMoved\":" << (cl.vBisBeingMoved ? "true" : "false");
		out << ",\"color\":";
		appendColor(out, cl.color);
		out << ",\"apparentColor\":";
		appendColor(out, cl.apparentColor);
		out << ",\"isSelected\":" << (cl.isSelected ? "true" : "false");
		out << ",\"spread\":" << cl.spread;
		out << "}";
	}
	out << "]}";

	out << "}";

	g_sceneJson = out.str();
	return static_cast<int>(g_sceneJson.size());
}

EMSCRIPTEN_KEEPALIVE const char* web_get_scene_json_ptr() {
	return g_sceneJson.empty() ? nullptr : g_sceneJson.c_str();
}

EMSCRIPTEN_KEEPALIVE int web_load_scene_json(const char* json) {
	if (!json || !json[0]) {
		return 0;
	}
	YAML::Node root;
	try {
		root = YAML::Load(json);
	}
	catch (const YAML::Exception&) {
		return 0;
	}

	const YAML::Node cameraNode = root["camera"];
	if (cameraNode) {
		const YAML::Node targetNode = cameraNode["target"];
		if (targetNode && targetNode.IsSequence()) {
			myParam.myCamera.camera.target = readVector2(targetNode);
		}
		const YAML::Node offsetNode = cameraNode["offset"];
		if (offsetNode && offsetNode.IsSequence()) {
			myParam.myCamera.camera.offset = readVector2(offsetNode);
		}
		const YAML::Node zoomNode = cameraNode["zoom"];
		if (zoomNode) {
			myParam.myCamera.camera.zoom = zoomNode.as<float>(myParam.myCamera.camera.zoom);
		}
		const YAML::Node rotationNode = cameraNode["rotation"];
		if (rotationNode) {
			myParam.myCamera.camera.rotation = rotationNode.as<float>(myParam.myCamera.camera.rotation);
		}
	}

	lighting.walls.clear();
	lighting.shapes.clear();
	lighting.pointLights.clear();
	lighting.areaLights.clear();
	lighting.coneLights.clear();

	const YAML::Node wallsNode = root["walls"];
	if (wallsNode && wallsNode.IsSequence()) {
		for (const auto& node : wallsNode) {
			const glm::vec2 vA = readVec2(node["vA"]);
			const glm::vec2 vB = readVec2(node["vB"]);
			const Color baseColor = readColor(node["baseColor"], WHITE);
			const Color specularColor = readColor(node["specularColor"], WHITE);
			const Color refractionColor = readColor(node["refractionColor"], WHITE);
			const Color emissionColor = readColor(node["emissionColor"], { 255, 255, 255, 0 });
			const float specularRoughness = node["specularRoughness"].as<float>(lighting.wallSpecularRoughness);
			const float refractionRoughness = node["refractionRoughness"].as<float>(lighting.wallRefractionRoughness);
			const float refractionAmount = node["refractionAmount"].as<float>(lighting.wallRefractionAmount);
			const float ior = node["ior"].as<float>(lighting.wallIOR);
			const float dispersion = node["dispersion"].as<float>(lighting.wallDispersion);
			const bool isShapeWall = node["isShapeWall"].as<bool>(false);

			lighting.walls.emplace_back(vA, vB, isShapeWall, baseColor, specularColor, refractionColor, emissionColor,
				specularRoughness, refractionRoughness, refractionAmount, ior, dispersion);
			Wall& w = lighting.walls.back();
			w.normal = readVec2(node["normal"]);
			w.normalVA = readVec2(node["normalVA"]);
			w.normalVB = readVec2(node["normalVB"]);
			w.isBeingSpawned = node["isBeingSpawned"].as<bool>(false);
			w.vAisBeingMoved = node["vAisBeingMoved"].as<bool>(false);
			w.vBisBeingMoved = node["vBisBeingMoved"].as<bool>(false);
			w.apparentColor = readColor(node["apparentColor"], WHITE);
			w.baseColorVal = node["baseColorVal"].as<float>(w.baseColorVal);
			w.specularColorVal = node["specularColorVal"].as<float>(w.specularColorVal);
			w.refractionColorVal = node["refractionColorVal"].as<float>(w.refractionColorVal);
			w.isShapeWall = node["isShapeWall"].as<bool>(w.isShapeWall);
			w.isShapeClosed = node["isShapeClosed"].as<bool>(false);
			w.shapeId = node["shapeId"].as<uint32_t>(0);
			w.id = node["id"].as<uint32_t>(w.id);
			w.isSelected = node["isSelected"].as<bool>(false);
		}
	}

	const YAML::Node shapesNode = root["shapes"];
	if (shapesNode && shapesNode.IsSequence()) {
		for (const auto& node : shapesNode) {
			const int type = node["type"].as<int>(0);
			const glm::vec2 h1 = readVec2(node["h1"]);
			const glm::vec2 h2 = readVec2(node["h2"]);
			const Color baseColor = readColor(node["baseColor"], WHITE);
			const Color specularColor = readColor(node["specularColor"], WHITE);
			const Color refractionColor = readColor(node["refractionColor"], WHITE);
			const Color emissionColor = readColor(node["emissionColor"], { 255, 255, 255, 0 });
			const float specularRoughness = node["specularRoughness"].as<float>(lighting.wallSpecularRoughness);
			const float refractionRoughness = node["refractionRoughness"].as<float>(lighting.wallRefractionRoughness);
			const float refractionAmount = node["refractionAmount"].as<float>(lighting.wallRefractionAmount);
			const float ior = node["ior"].as<float>(lighting.wallIOR);
			const float dispersion = node["dispersion"].as<float>(lighting.wallDispersion);

			Shape s(static_cast<ShapeType>(type), h1, h2, &lighting.walls,
				baseColor, specularColor, refractionColor, emissionColor,
				specularRoughness, refractionRoughness, refractionAmount, ior, dispersion);
			s.id = node["id"].as<uint32_t>(s.id);
			s.isBeingSpawned = node["isBeingSpawned"].as<bool>(false);
			s.isBeingMoved = node["isBeingMoved"].as<bool>(false);
			s.isShapeClosed = node["isShapeClosed"].as<bool>(true);
			s.drawHoverHelpers = node["drawHoverHelpers"].as<bool>(false);
			s.oldDrawHelperPos = readVec2(node["oldDrawHelperPos"]);
			s.symmetricalLens = node["symmetricalLens"].as<bool>(s.symmetricalLens);
			s.helpers = readVec2List(node["helpers"]);
			s.polygonVerts = readVec2List(node["polygon"]);
			s.myWallIds = readUintList(node["wallIds"]);
			s.secondHelper = node["secondHelper"].as<bool>(false);
			s.thirdHelper = node["thirdHelper"].as<bool>(false);
			s.fourthHelper = node["fourthHelper"].as<bool>(false);
			s.Tempsh2Length = node["Tempsh2Length"].as<float>(0.0f);
			s.Tempsh2LengthSymmetry = node["Tempsh2LengthSymmetry"].as<float>(0.0f);
			s.tempDist = node["tempDist"].as<float>(0.0f);
			s.moveH2 = readVec2(node["moveH2"]);
			s.isThirdBeingMoved = node["isThirdBeingMoved"].as<bool>(false);
			s.isFourthBeingMoved = node["isFourthBeingMoved"].as<bool>(false);
			s.isFifthBeingMoved = node["isFifthBeingMoved"].as<bool>(false);
			s.isFifthFourthMoved = node["isFifthFourthMoved"].as<bool>(false);
			s.wallAId = node["wallAId"].as<uint32_t>(s.wallAId);
			s.wallBId = node["wallBId"].as<uint32_t>(s.wallBId);
			s.wallCId = node["wallCId"].as<uint32_t>(s.wallCId);
			s.lensSegments = node["lensSegments"].as<int>(s.lensSegments);
			s.startAngle = node["startAngle"].as<float>(s.startAngle);
			s.endAngle = node["endAngle"].as<float>(s.endAngle);
			s.startAngleSymmetry = node["startAngleSymmetry"].as<float>(s.startAngleSymmetry);
			s.endAngleSymmetry = node["endAngleSymmetry"].as<float>(s.endAngleSymmetry);
			s.center = readVec2(node["center"]);
			s.radius = node["radius"].as<float>(s.radius);
			s.centerSymmetry = readVec2(node["centerSymmetry"]);
			s.radiusSymmetry = node["radiusSymmetry"].as<float>(s.radiusSymmetry);
			s.arcEnd = readVec2(node["arcEnd"]);
			s.globalLensPrev = readVec2(node["globalLensPrev"]);
			s.walls = &lighting.walls;

			bool hasValidWallIds = !s.myWallIds.empty();
			if (hasValidWallIds) {
				for (uint32_t wallId : s.myWallIds) {
					if (!s.getWallById(lighting.walls, wallId)) {
						hasValidWallIds = false;
						break;
					}
				}
			}

			if (!hasValidWallIds) {
				s.myWallIds.clear();
				const std::vector<glm::vec2>& polygon = s.polygonVerts;
				if (polygon.size() >= 2) {
					for (size_t i = 0; i < polygon.size(); ++i) {
						const glm::vec2 vA = polygon[i];
						const glm::vec2 vB = polygon[(i + 1) % polygon.size()];
						lighting.walls.emplace_back(vA, vB, true, baseColor, specularColor, refractionColor, emissionColor,
							specularRoughness, refractionRoughness, refractionAmount, ior, dispersion);
						Wall& w = lighting.walls.back();
						w.isBeingSpawned = false;
						w.vAisBeingMoved = false;
						w.vBisBeingMoved = false;
						w.isShapeWall = true;
						w.isShapeClosed = s.isShapeClosed;
						w.shapeId = s.id;
						w.isSelected = false;
						w.apparentColor = WHITE;
						s.myWallIds.push_back(w.id);
					}
				}
			} else {
				for (uint32_t wallId : s.myWallIds) {
					Wall* w = s.getWallById(lighting.walls, wallId);
					if (w) {
						w->isShapeWall = true;
						w->isShapeClosed = s.isShapeClosed;
						w->shapeId = s.id;
					}
				}
			}

			s.calculateWallsNormals();
			lighting.shapes.push_back(s);
		}
	}

	const YAML::Node lightsNode = root["lights"];
	if (lightsNode) {
		const YAML::Node pointNode = lightsNode["point"];
		if (pointNode && pointNode.IsSequence()) {
			for (const auto& node : pointNode) {
				const glm::vec2 pos = readVec2(node["pos"]);
				const Color color = readColor(node["color"], WHITE);
				PointLight p(pos, color);
				p.isBeingMoved = node["isBeingMoved"].as<bool>(false);
				p.apparentColor = readColor(node["apparentColor"], WHITE);
				p.isSelected = node["isSelected"].as<bool>(false);
				lighting.pointLights.push_back(p);
			}
		}

		const YAML::Node areaNode = lightsNode["area"];
		if (areaNode && areaNode.IsSequence()) {
			for (const auto& node : areaNode) {
				const glm::vec2 vA = readVec2(node["vA"]);
				const glm::vec2 vB = readVec2(node["vB"]);
				const Color color = readColor(node["color"], WHITE);
				const float spread = node["spread"].as<float>(lighting.lightSpread);
				AreaLight a(vA, vB, color, spread);
				a.isBeingSpawned = node["isBeingSpawned"].as<bool>(false);
				a.vAisBeingMoved = node["vAisBeingMoved"].as<bool>(false);
				a.vBisBeingMoved = node["vBisBeingMoved"].as<bool>(false);
				a.apparentColor = readColor(node["apparentColor"], WHITE);
				a.isSelected = node["isSelected"].as<bool>(false);
				lighting.areaLights.push_back(a);
			}
		}

		const YAML::Node coneNode = lightsNode["cone"];
		if (coneNode && coneNode.IsSequence()) {
			for (const auto& node : coneNode) {
				const glm::vec2 vA = readVec2(node["vA"]);
				const glm::vec2 vB = readVec2(node["vB"]);
				const Color color = readColor(node["color"], WHITE);
				const float spread = node["spread"].as<float>(lighting.lightSpread);
				ConeLight c(vA, vB, color, spread);
				c.isBeingSpawned = node["isBeingSpawned"].as<bool>(false);
				c.vAisBeingMoved = node["vAisBeingMoved"].as<bool>(false);
				c.vBisBeingMoved = node["vBisBeingMoved"].as<bool>(false);
				c.apparentColor = readColor(node["apparentColor"], WHITE);
				c.isSelected = node["isSelected"].as<bool>(false);
				lighting.coneLights.push_back(c);
			}
		}
	}

	uint32_t maxWallId = 0;
	for (const Wall& w : lighting.walls) {
		if (w.id > maxWallId) {
			maxWallId = w.id;
		}
	}
	globalWallId = maxWallId + 1;

	uint32_t maxShapeId = 0;
	for (const Shape& s : lighting.shapes) {
		if (s.id > maxShapeId) {
			maxShapeId = s.id;
		}
	}
	globalShapeId = maxShapeId + 1;

	for (Wall& w : lighting.walls) {
		if (!w.isShapeWall) {
			lighting.calculateWallNormal(w);
			w.normalVA = w.normal;
			w.normalVB = w.normal;
		}
	}

	lighting.shouldRender = true;
	resetCameraAfterLoad();
	logSceneLoadSummary("json");
	compareWithLastSaved("json");
	compareWithDefaults("json");
	compareUpdateVariablesWithDefaults("json");
	return 1;
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

EMSCRIPTEN_KEEPALIVE int web_get_fps() {
	return GetFPS();
}

EMSCRIPTEN_KEEPALIVE float web_get_frame_time() {
	return GetFrameTime();
}

EMSCRIPTEN_KEEPALIVE int web_get_particle_count() {
	return static_cast<int>(myParam.pParticles.size());
}

EMSCRIPTEN_KEEPALIVE int web_get_selected_particle_count() {
	return static_cast<int>(myParam.pParticlesSelected.size());
}

EMSCRIPTEN_KEEPALIVE int web_get_total_lights() {
	return lighting.totalLights;
}

EMSCRIPTEN_KEEPALIVE int web_get_accumulated_rays() {
	return lighting.accumulatedRays;
}

EMSCRIPTEN_KEEPALIVE void web_set_dark_matter_enabled(int enabled) {
	myVar.isDarkMatterEnabled = (enabled != 0);
}

EMSCRIPTEN_KEEPALIVE int web_get_dark_matter_enabled() {
	return myVar.isDarkMatterEnabled ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_looping_space_enabled(int enabled) {
	myVar.isPeriodicBoundaryEnabled = (enabled != 0);
}

EMSCRIPTEN_KEEPALIVE int web_get_looping_space_enabled() {
	return myVar.isPeriodicBoundaryEnabled ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_fluid_ground_enabled(int enabled) {
	myVar.sphGround = (enabled != 0);
}

EMSCRIPTEN_KEEPALIVE int web_get_fluid_ground_enabled() {
	return myVar.sphGround ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_temperature_enabled(int enabled) {
	myVar.isTempEnabled = (enabled != 0);
}

EMSCRIPTEN_KEEPALIVE int web_get_temperature_enabled() {
	return myVar.isTempEnabled ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_highlight_selected(int enabled) {
	myParam.colorVisuals.selectedColor = (enabled != 0);
}

EMSCRIPTEN_KEEPALIVE int web_get_highlight_selected() {
	return myParam.colorVisuals.selectedColor ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_constraints_enabled(int enabled) {
	myVar.constraintsEnabled = (enabled != 0);
}

EMSCRIPTEN_KEEPALIVE int web_get_constraints_enabled() {
	return myVar.constraintsEnabled ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_unbreakable_constraints(int enabled) {
	myVar.unbreakableConstraints = (enabled != 0);
}

EMSCRIPTEN_KEEPALIVE int web_get_unbreakable_constraints() {
	return myVar.unbreakableConstraints ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_constraint_after_drawing(int enabled) {
	myVar.constraintAfterDrawing = (enabled != 0);
}

EMSCRIPTEN_KEEPALIVE int web_get_constraint_after_drawing() {
	return myVar.constraintAfterDrawing ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_draw_constraints(int enabled) {
	myVar.drawConstraints = (enabled != 0);
	if (myVar.drawConstraints) {
		myVar.visualizeMesh = false;
	}
}

EMSCRIPTEN_KEEPALIVE int web_get_draw_constraints() {
	return myVar.drawConstraints ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_visualize_mesh(int enabled) {
	myVar.visualizeMesh = (enabled != 0);
	if (myVar.visualizeMesh) {
		myVar.drawConstraints = false;
	}
}

EMSCRIPTEN_KEEPALIVE int web_get_visualize_mesh() {
	return myVar.visualizeMesh ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_constraint_stress_color(int enabled) {
	myVar.constraintStressColor = (enabled != 0);
}

EMSCRIPTEN_KEEPALIVE int web_get_constraint_stress_color() {
	return myVar.constraintStressColor ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_gravity_field_enabled(int enabled) {
	bool next = (enabled != 0);
	if (myVar.isGravityFieldEnabled != next) {
		myVar.isGravityFieldEnabled = next;
		field.computeField = true;
	}
}

EMSCRIPTEN_KEEPALIVE int web_get_gravity_field_enabled() {
	return myVar.isGravityFieldEnabled ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_gravity_field_dm_particles(int enabled) {
	myVar.gravityFieldDMParticles = (enabled != 0);
}

EMSCRIPTEN_KEEPALIVE int web_get_gravity_field_dm_particles() {
	return myVar.gravityFieldDMParticles ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_field_res(int res) {
	if (res < 50) {
		res = 50;
	}
	if (res > 1000) {
		res = 1000;
	}
	if (field.res != res) {
		field.res = res;
		field.computeField = true;
	}
}

EMSCRIPTEN_KEEPALIVE int web_get_field_res() {
	return field.res;
}

EMSCRIPTEN_KEEPALIVE void web_set_gravity_display_threshold(float threshold) {
	if (threshold < 10.0f) {
		threshold = 10.0f;
	}
	if (threshold > 3000.0f) {
		threshold = 3000.0f;
	}
	field.gravityDisplayThreshold = threshold;
}

EMSCRIPTEN_KEEPALIVE float web_get_gravity_display_threshold() {
	return field.gravityDisplayThreshold;
}

EMSCRIPTEN_KEEPALIVE void web_set_gravity_display_softness(float softness) {
	if (softness < 0.4f) {
		softness = 0.4f;
	}
	if (softness > 8.0f) {
		softness = 8.0f;
	}
	field.gravityDisplaySoftness = softness;
}

EMSCRIPTEN_KEEPALIVE float web_get_gravity_display_softness() {
	return field.gravityDisplaySoftness;
}

EMSCRIPTEN_KEEPALIVE void web_set_gravity_display_stretch(float stretch) {
	if (stretch < 1.0f) {
		stretch = 1.0f;
	}
	if (stretch > 10000.0f) {
		stretch = 10000.0f;
	}
	field.gravityStretchFactor = stretch;
}

EMSCRIPTEN_KEEPALIVE float web_get_gravity_display_stretch() {
	return field.gravityStretchFactor;
}

EMSCRIPTEN_KEEPALIVE void web_set_gravity_custom_colors(int enabled) {
	field.gravityCustomColors = (enabled != 0);
}

EMSCRIPTEN_KEEPALIVE int web_get_gravity_custom_colors() {
	return field.gravityCustomColors ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_gravity_exposure(float exposure) {
	if (exposure < 0.001f) {
		exposure = 0.001f;
	}
	if (exposure > 15.0f) {
		exposure = 15.0f;
	}
	field.gravityExposure = exposure;
}

EMSCRIPTEN_KEEPALIVE float web_get_gravity_exposure() {
	return field.gravityExposure;
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
#if defined(EMSCRIPTEN)
	if (threads > 4) {
		threads = 4;
	}
#else
	if (threads > 32) {
		threads = 32;
	}
#endif
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

EMSCRIPTEN_KEEPALIVE void web_set_path_prediction_enabled(int enabled) {
	myParam.particlesSpawning.enablePathPrediction = (enabled != 0);
}

EMSCRIPTEN_KEEPALIVE int web_get_path_prediction_enabled() {
	return myParam.particlesSpawning.enablePathPrediction ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void web_set_path_prediction_length(int length) {
	if (length < 100) {
		length = 100;
	}
	if (length > 2000) {
		length = 2000;
	}
	myParam.particlesSpawning.predictPathLength = length;
}

EMSCRIPTEN_KEEPALIVE int web_get_path_prediction_length() {
	return myParam.particlesSpawning.predictPathLength;
}

EMSCRIPTEN_KEEPALIVE void web_set_particle_amount_multiplier(float mult) {
	if (mult < 0.1f) {
		mult = 0.1f;
	}
	if (mult > 100.0f) {
		mult = 100.0f;
	}
	myParam.particlesSpawning.particleAmountMultiplier = mult;
}

EMSCRIPTEN_KEEPALIVE float web_get_particle_amount_multiplier() {
	return myParam.particlesSpawning.particleAmountMultiplier;
}

EMSCRIPTEN_KEEPALIVE void web_set_dark_matter_amount_multiplier(float mult) {
	if (mult < 0.1f) {
		mult = 0.1f;
	}
	if (mult > 100.0f) {
		mult = 100.0f;
	}
	myParam.particlesSpawning.DMAmountMultiplier = mult;
}

EMSCRIPTEN_KEEPALIVE float web_get_dark_matter_amount_multiplier() {
	return myParam.particlesSpawning.DMAmountMultiplier;
}

EMSCRIPTEN_KEEPALIVE void web_set_mass_multiplier_enabled(int enabled) {
	if (myVar.isSPHEnabled) {
		myParam.particlesSpawning.massMultiplierEnabled = false;
		return;
	}
	myParam.particlesSpawning.massMultiplierEnabled = (enabled != 0);
}

EMSCRIPTEN_KEEPALIVE int web_get_mass_multiplier_enabled() {
	return myParam.particlesSpawning.massMultiplierEnabled ? 1 : 0;
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
