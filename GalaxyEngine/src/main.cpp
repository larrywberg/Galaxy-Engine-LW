#include "globalLogic.h"
#if defined(EMSCRIPTEN)
#include <emscripten/emscripten.h>
#endif

int main(int argc, char** argv) {

	// SPH Materials initialization
	SPHMaterials::Init();

	SetTraceLogLevel(LOG_WARNING);

	SetConfigFlags(FLAG_MSAA_4X_HINT);

	SetConfigFlags(FLAG_WINDOW_RESIZABLE);

	SetConfigFlags(FLAG_WINDOW_ALWAYS_RUN);

	int threadsAvailable = static_cast<int>(std::thread::hardware_concurrency());
	if (threadsAvailable <= 0) {
		threadsAvailable = 1;
	}

#if defined(EMSCRIPTEN)
	myVar.threadsAmount = 4;
#else
	myVar.threadsAmount = std::max(1, static_cast<int>(threadsAvailable * 0.5f));
#endif

	std::cout << "Threads available: " << threadsAvailable << std::endl;
	std::cout << "Thread amount set to: " << myVar.threadsAmount << std::endl;

	if (myVar.fullscreenState) {
		myVar.screenWidth = GetMonitorWidth(GetCurrentMonitor());
		myVar.screenHeight = GetMonitorHeight(GetCurrentMonitor());
	}

	InitWindow(myVar.screenWidth, myVar.screenHeight, "Galaxy Engine");
	myVar.screenWidth = GetScreenWidth();
	myVar.screenHeight = GetScreenHeight();
	myVar.halfScreenWidth = myVar.screenWidth * 0.5f;
	myVar.halfScreenHeight = myVar.screenHeight * 0.5f;

	// ---- Config ---- //

	if (!std::filesystem::exists("Config")) {
		std::filesystem::create_directory("Config");
	}

	if (!std::filesystem::exists("Config/config.txt")) {
		saveConfig();
	}
	else {
		loadConfig();
	}

	// ---- Audio ---- //

	geSound.loadSounds();

	// ---- Icon ---- //

	Image icon = LoadImage("Textures/WindowIcon.png");
	SetWindowIcon(icon);

	// ---- Textures & rlImGui ---- //
#if !defined(EMSCRIPTEN)
	rlImGuiSetup(true);
#else
	myVar.isMouseNotHoveringUI = true;
#endif

	Texture2D particleBlurTex = LoadTexture("Textures/ParticleBlur.png");

#if defined(EMSCRIPTEN)
	const char* bloomFs = R"(
#version 100

precision mediump float;
precision mediump sampler2D;

varying vec2 fragTexCoord;
varying vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec2 resolution;

const float samples = 5.0;
const float quality = 2.5;

void main()
{
    vec4 sum = vec4(0.0);
    vec2 sizeFactor = (vec2(1.0) / resolution) * quality;
    vec4 source = texture2D(texture0, fragTexCoord);

    const int range = 2;

    for (int x = -range; x <= range; x++)
    {
        for (int y = -range; y <= range; y++)
        {
            sum += texture2D(texture0, fragTexCoord + vec2(float(x), float(y)) * sizeFactor);
        }
    }

    vec4 blurred = sum / (samples * samples);
    gl_FragColor = (blurred + source) * colDiffuse * fragColor;
}
	)";
	Shader myBloom = LoadShaderFromMemory(nullptr, bloomFs);
	int bloomResolutionLoc = GetShaderLocation(myBloom, "resolution");
	float bloomResolution[2] = { (float)GetScreenWidth(), (float)GetScreenHeight() };
	const bool useBloom = (myBloom.id != 0);
	if (useBloom && bloomResolutionLoc != -1) {
		SetShaderValue(myBloom, bloomResolutionLoc, bloomResolution, SHADER_UNIFORM_VEC2);
	}
#else
	Shader myBloom = LoadShader(nullptr, "Shaders/bloom.fs");
	int bloomResolutionLoc = -1;
	float bloomResolution[2] = { 0.0f, 0.0f };
	const bool useBloom = (myBloom.id != 0);
#endif

	RenderTexture2D myParticlesTexture = CreateFloatRenderTexture(GetScreenWidth(), GetScreenHeight());
	RenderTexture2D myRayTracingTexture = CreateFloatRenderTexture(GetScreenWidth(), GetScreenHeight());
	RenderTexture2D myUITexture = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
	RenderTexture2D myMiscTexture = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());

	SetTargetFPS(myVar.targetFPS);

	// ---- Fullscreen ---- //

	int lastScreenWidth = GetScreenWidth();
	int lastScreenHeight = GetScreenHeight();
	bool wasFullscreen = IsWindowMaximized();

	bool lastScreenState = false;

	// ---- Save ---- //

	// If "Saves" directory doesn't exist, then create one. This is done here to store the default parameters
	if (!std::filesystem::exists("Saves")) {
		std::filesystem::create_directory("Saves");
	}

	save.saveFlag = true;
	save.saveSystem("Saves/DefaultSettings.bin", myVar, myParam, sph, physics, lighting, field);
	save.saveFlag = false;

	// ---- ImGui ---- //
#if !defined(EMSCRIPTEN)
	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4* colors = style.Colors;

	// Button and window colors
	colors[ImGuiCol_WindowBg] = myVar.colWindowBg;
	colors[ImGuiCol_Button] = myVar.colButton;
	colors[ImGuiCol_ButtonHovered] = myVar.colButtonHover;
	colors[ImGuiCol_ButtonActive] = myVar.colButtonPress;

	// Slider colors
	style.Colors[ImGuiCol_SliderGrab] = myVar.colSliderGrab;        // Bright cyan-ish knob
	style.Colors[ImGuiCol_SliderGrabActive] = myVar.colSliderGrabActive;  // Darker when dragging

	style.Colors[ImGuiCol_FrameBg] = myVar.colSliderBg;           // Dark track when idle
	style.Colors[ImGuiCol_FrameBgHovered] = myVar.colSliderBgHover;    // Lighter track on hover
	style.Colors[ImGuiCol_FrameBgActive] = myVar.colSliderBgActive;     // Even lighter on active

	// Tab colors
	style.Colors[ImGuiCol_Tab] = myVar.colButton;
	style.Colors[ImGuiCol_TabHovered] = myVar.colButtonHover;
	style.Colors[ImGuiCol_TabActive] = myVar.colButtonPress;

	ImGuiIO& io = ImGui::GetIO();

	ImFontConfig config;
	config.SizePixels = 14.0f;        // Base font size in pixels
	config.RasterizerDensity = 2.0f;  // Improves rendering at small sizes
	config.OversampleH = 4;           // Horizontal anti-aliasing
	config.OversampleV = 4;           // Vertical anti-aliasing
	config.PixelSnapH = false;        // Disable pixel snapping for smoother text
	config.RasterizerMultiply = 0.9f; // Slightly boosts brightness

	myVar.robotoMediumFont = io.Fonts->AddFontFromFileTTF(
		"fonts/Roboto-Medium.ttf",
		config.SizePixels,
		&config,
		io.Fonts->GetGlyphRangesDefault()
	);

	if (!myVar.robotoMediumFont) {
		std::cerr << "Failed to load special font!\n";
	}
	else {
		std::cout << "Special font loaded successfully\n";
	}

	io.Fonts->Build();
#endif
#if !defined(EMSCRIPTEN)
	ImPlot::CreateContext();
#endif

	// ---- Intro ---- //

	bool fadeActive = false;
	bool introActive = false;

	myVar.customFont = LoadFontEx("fonts/Unispace Bd.otf", myVar.introFontSize, 0, 250);

	SetTextureFilter(myVar.customFont.texture, TEXTURE_FILTER_BILINEAR);

	if (myVar.customFont.texture.id == 0) {
		TraceLog(LOG_WARNING, "Failed to load font! Using default font");
	}

	if (myVar.fullscreenState) {
		myVar.screenWidth = GetMonitorWidth(GetCurrentMonitor()) * 0.5f;
		myVar.screenHeight = GetMonitorHeight(GetCurrentMonitor()) * 0.5f;
	}

	// ---- Ray Tracing and Long Exposure ---- //

#if defined(EMSCRIPTEN)
	const char* accumulationVs = R"(
    #version 100

attribute vec3 vertexPosition;
attribute vec2 vertexTexCoord;
attribute vec4 vertexColor;

varying vec2 fragTexCoord;
varying vec4 fragColor;

uniform mat4 mvp;

void main()
{
    gl_Position = mvp * vec4(vertexPosition, 1.0);
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
}
    )";

	const char* accumulationFs = R"(
#version 100

precision mediump float;
precision mediump sampler2D;

varying vec2 fragTexCoord;
varying vec4 fragColor;

uniform sampler2D currentFrame;
uniform sampler2D accumulatedFrame;
uniform float sampleCount;

void main() {

    vec4 newColor = texture2D(currentFrame, fragTexCoord);
    vec4 oldColor = texture2D(accumulatedFrame, fragTexCoord);

    if (sampleCount <= 1.0) {
        gl_FragColor = clamp(newColor, 0.0, 1.0) * fragColor;
        return;
    }

    vec4 combined = (oldColor * (sampleCount - 1.0) + newColor) / sampleCount;

    gl_FragColor = clamp(combined, 0.0, 1.0) * fragColor;
}
)";
#else
	const char* accumulationVs = R"(
    #version 330 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec2 vertexTexCoord;
layout(location = 3) in vec4 vertexColor;

out vec2 fragTexCoord;
out vec4 fragColor;

uniform mat4 mvp;

void main()
{

    gl_Position = mvp * vec4(vertexPosition, 1.0);


    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;

}
    )";

	const char* accumulationFs = R"(

#version 330

precision highp float;
precision highp sampler2D;

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D currentFrame;
uniform sampler2D accumulatedFrame;
uniform float sampleCount;

void main() {

    highp vec4 newColor = texture(currentFrame, fragTexCoord);
    highp vec4 oldColor = texture(accumulatedFrame, fragTexCoord);

    if (sampleCount <= 1.0) {
        finalColor = clamp(newColor, 0.0, 1.0);
        return;
    }

    finalColor = (oldColor * (sampleCount - 1.0) + newColor) / sampleCount;

    finalColor = clamp(finalColor, 0.0, 1.0);
}
)";
#endif

#if defined(EMSCRIPTEN)
	Shader accumulationShader = LoadShaderFromMemory(accumulationVs, accumulationFs);
	const bool useAccumulationShader = (accumulationShader.id != 0);

	int screenSizeLoc = GetShaderLocation(accumulationShader, "screenSize");
	float screenSize[2] = {
		(float)myVar.screenWidth,
		(float)myVar.screenHeight
	};
	if (screenSizeLoc != -1) {
		SetShaderValue(accumulationShader, screenSizeLoc, screenSize, SHADER_UNIFORM_VEC2);
	}

	int rayTextureLoc = GetShaderLocation(accumulationShader, "rayTexture");

	RenderTexture2D accumulatedTexture = CreateFloatRenderTexture(GetScreenWidth(), GetScreenHeight());
	RenderTexture2D pingPongTexture = CreateFloatRenderTexture(GetScreenWidth(), GetScreenHeight());

	int currentFrameLoc = GetShaderLocation(accumulationShader, "currentFrame");
	int accumulatedFrameLoc = GetShaderLocation(accumulationShader, "accumulatedFrame");
	int sampleCountLoc = GetShaderLocation(accumulationShader, "sampleCount");
	RenderTexture2D testSampleTexture = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
#else
	Shader accumulationShader = LoadShaderFromMemory(accumulationVs, accumulationFs);
	const bool useAccumulationShader = (accumulationShader.id != 0);

	int screenSizeLoc = GetShaderLocation(accumulationShader, "screenSize");
	float screenSize[2] = {
		(float)myVar.screenWidth,
		(float)myVar.screenHeight
	};
	SetShaderValue(accumulationShader, screenSizeLoc, screenSize, SHADER_UNIFORM_VEC2);

	int rayTextureLoc = GetShaderLocation(accumulationShader, "rayTexture");

	RenderTexture2D accumulatedTexture = CreateFloatRenderTexture(GetScreenWidth(), GetScreenHeight());
	RenderTexture2D pingPongTexture = CreateFloatRenderTexture(GetScreenWidth(), GetScreenHeight());

	int currentFrameLoc = GetShaderLocation(accumulationShader, "currentFrame");
	int accumulatedFrameLoc = GetShaderLocation(accumulationShader, "accumulatedFrame");
	int sampleCountLoc = GetShaderLocation(accumulationShader, "sampleCount");
	RenderTexture2D testSampleTexture = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
#endif

	int prevScreenWidth = GetScreenWidth();
	int prevScreenHeight = GetScreenHeight();

	bool prevLongExpFlag = false;

	bool accumulationCondition = false;

	buildKernels();

	while (!WindowShouldClose()) {

		if (myVar.exitGame) {
			CloseWindow();
			break;
		}

		fullscreenToggle(lastScreenWidth, lastScreenHeight, wasFullscreen, lastScreenState, myParticlesTexture, myUITexture);

		BeginTextureMode(myParticlesTexture);

		ClearBackground(BLACK);

		BeginBlendMode(myParam.colorVisuals.blendMode);

		BeginMode2D(myParam.myCamera.cameraLogic(save.loadFlag, myVar.isMouseNotHoveringUI));

#if !defined(EMSCRIPTEN)
		rlImGuiBegin();
#endif

		if (introActive) {
			ImGuiIO& io = ImGui::GetIO();
			io.WantCaptureMouse = true;
			io.WantCaptureKeyboard = true;
			io.WantTextInput = true;

			if (myParam.pParticles.size() > 0) {
				myParam.pParticles.clear();
				myParam.rParticles.clear();
			}
		}
		else {
			geSound.soundtrackLogic();
		}

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);

		saveConfigIfChanged();

		updateScene();

		drawScene(particleBlurTex, myRayTracingTexture, myUITexture, myMiscTexture, fadeActive, introActive);

		EndMode2D();

		EndBlendMode();


		//------------------------ RENDER TEXTURES BELOW ------------------------//

		if (myParam.myCamera.cameraChangedThisFrame) {
			if (!myVar.isOpticsEnabled || lighting.currentSamples <= lighting.maxSamples) {
				lighting.shouldRender = true;
				lighting.accumulationResetRequested = true;
			}
		}

		if (myVar.longExposureFlag != prevLongExpFlag) {
			myVar.longExposureCurrent = 1;

			prevLongExpFlag = myVar.longExposureFlag;
		}

		if (useAccumulationShader && myVar.isOpticsEnabled) {
			accumulationCondition = lighting.currentSamples <= lighting.maxSamples;
		}
		else if (useAccumulationShader && myVar.longExposureFlag) {
			accumulationCondition = myVar.longExposureCurrent <= myVar.longExposureDuration;
		}
		else {
			accumulationCondition = useAccumulationShader;
		}

		if (GetScreenWidth() != prevScreenWidth || GetScreenHeight() != prevScreenHeight) {
			myVar.screenWidth = GetScreenWidth();
			myVar.screenHeight = GetScreenHeight();
			myVar.halfScreenWidth = myVar.screenWidth * 0.5f;
			myVar.halfScreenHeight = myVar.screenHeight * 0.5f;
			const float deltaX = (static_cast<float>(GetScreenWidth()) - static_cast<float>(prevScreenWidth)) * 0.5f;
			const float deltaY = (static_cast<float>(GetScreenHeight()) - static_cast<float>(prevScreenHeight)) * 0.5f;
			// Keep the camera offset anchored relative to the screen center after resize.
			myParam.myCamera.camera.offset.x += deltaX;
			myParam.myCamera.camera.offset.y += deltaY;

			UnloadRenderTexture(accumulatedTexture);
			accumulatedTexture = CreateFloatRenderTexture(GetScreenWidth(), GetScreenHeight());
			UnloadRenderTexture(pingPongTexture);
			UnloadRenderTexture(testSampleTexture);

			pingPongTexture = CreateFloatRenderTexture(GetScreenWidth(), GetScreenHeight());
			testSampleTexture = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());

			screenSize[0] = (float)GetScreenWidth();
			screenSize[1] = (float)GetScreenHeight();
			if (screenSizeLoc != -1) {
				SetShaderValue(accumulationShader, screenSizeLoc, screenSize, SHADER_UNIFORM_VEC2);
			}
			if (useBloom && bloomResolutionLoc != -1) {
				bloomResolution[0] = (float)GetScreenWidth();
				bloomResolution[1] = (float)GetScreenHeight();
				SetShaderValue(myBloom, bloomResolutionLoc, bloomResolution, SHADER_UNIFORM_VEC2);
			}

			prevScreenWidth = GetScreenWidth();
			prevScreenHeight = GetScreenHeight();

			if (useAccumulationShader && myVar.isOpticsEnabled) {
				lighting.shouldRender = true;
				lighting.accumulationResetRequested = true;
			}

			if (useAccumulationShader && myVar.longExposureFlag) {
				myVar.longExposureFlag = false;
			}
		}

		if (lighting.accumulationResetRequested) {
			BeginTextureMode(accumulatedTexture);
			ClearBackground(BLACK);
			EndTextureMode();
			BeginTextureMode(pingPongTexture);
			ClearBackground(BLACK);
			EndTextureMode();
			lighting.accumulationResetRequested = false;
		}

		// Ray Tracing and Long Exposure
		if (accumulationCondition) {

			BeginTextureMode(pingPongTexture);

			BeginShaderMode(accumulationShader);

			SetShaderValueTexture(accumulationShader, currentFrameLoc, myParticlesTexture.texture);
			SetShaderValueTexture(accumulationShader, accumulatedFrameLoc, accumulatedTexture.texture);

			float sampleCount = 1.0f;
			if (myVar.isOpticsEnabled) {
				sampleCount = static_cast<float>(lighting.currentSamples);
			}
			else {
				lighting.currentSamples = 0;
			}

			if (myVar.longExposureFlag) {
				sampleCount = static_cast<float>(myVar.longExposureCurrent);
			}
			else {
				myVar.longExposureCurrent = 0;
			}

			if (sampleCount < 1.0f) {
				sampleCount = 1.0f;
			}

			SetShaderValue(accumulationShader, sampleCountLoc, &sampleCount, SHADER_UNIFORM_FLOAT);

			DrawTextureRec(
				accumulatedTexture.texture,
				Rectangle{ 0, 0, (float)GetScreenWidth(), -((float)GetScreenHeight()) },
				Vector2{ 0, 0 },
				WHITE
			);

			EndShaderMode();

			EndTextureMode();

			std::swap(accumulatedTexture, pingPongTexture);


			if (myVar.longExposureFlag) {
				myVar.longExposureCurrent++;
			}
		}
#if defined(EMSCRIPTEN)
		if (!useAccumulationShader) {
			BeginTextureMode(accumulatedTexture);
			ClearBackground(BLACK);
			DrawTextureRec(
				myParticlesTexture.texture,
				Rectangle{ 0, 0, (float)GetScreenWidth(), -((float)GetScreenHeight()) },
				Vector2{ 0, 0 },
				WHITE
			);
			EndTextureMode();
		}
#endif

		if (useBloom && myVar.isGlowEnabled) {
			BeginShaderMode(myBloom);
		}
		DrawTextureRec(
			accumulatedTexture.texture,
			Rectangle{ 0, 0, (float)GetScreenWidth(), -((float)GetScreenHeight()) },
			Vector2{ 0, 0 },
			WHITE
		);
		if (useBloom && myVar.isGlowEnabled) {
			EndShaderMode();
		}


		DrawTextureRec(
			myUITexture.texture,
			Rectangle{ 0, 0, static_cast<float>(GetScreenWidth()), -static_cast<float>(GetScreenHeight()) },
			Vector2{ 0, 0 },
			WHITE
		);

		EndBlendMode();


		// Detects if the user is recording the screen
		myVar.isRecording = myParam.screenCapture.screenGrab(accumulatedTexture, myVar, myParam);

		if (myVar.isRecording) {
			DrawRectangleLinesEx({ 0,0, static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight()) }, 3, RED);
		}

		ImGui::PopStyleVar();

#if !defined(EMSCRIPTEN)
		rlImGuiEnd();
#endif

		DrawTextureRec(
			myMiscTexture.texture,
			Rectangle{ 0, 0, static_cast<float>(GetScreenWidth()), -static_cast<float>(GetScreenHeight()) },
			Vector2{ 0, 0 },
			WHITE
		);


		EndDrawing();

#if defined(EMSCRIPTEN)
		EM_ASM({
			if (typeof window !== 'undefined' && window.webFrameCaptureTick) {
				if (Module && Module.ctx && Module.ctx.finish) {
					Module.ctx.finish();
				}
				window.webFrameCaptureTick();
			}
		});
#endif

		enableMultiThreading();
	}

#if !defined(EMSCRIPTEN)
	rlImGuiShutdown();
	ImPlot::DestroyContext();
#endif

#if !defined(EMSCRIPTEN)
	UnloadShader(myBloom);
	UnloadShader(accumulationShader);
#endif
	UnloadTexture(particleBlurTex);

	UnloadRenderTexture(myParticlesTexture);
	UnloadRenderTexture(myRayTracingTexture);
	UnloadRenderTexture(myUITexture);
	UnloadRenderTexture(myMiscTexture);

	UnloadImage(icon);

	geSound.unloadSounds();

	// Unload accumulation shader
#if !defined(EMSCRIPTEN)
	UnloadRenderTexture(pingPongTexture);
	UnloadRenderTexture(testSampleTexture);
#endif

	// Free compute shader memory
	freeGPUMemory();

	CloseWindow();



	return 0;
}
