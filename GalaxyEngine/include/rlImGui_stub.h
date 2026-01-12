#pragma once

#include <imgui.h>
#include <raylib.h>

// WebAssembly ImGui renderer hooks implemented in rlImGui_wasm.cpp.
void rlImGuiSetup(bool darkStyle);
void rlImGuiBegin();
void rlImGuiEnd();
void rlImGuiShutdown();
