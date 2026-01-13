#pragma once

// Input/Output utility functions
namespace IO {
    
    // Handle keyboard shortcuts with ImGui integration
    static inline bool shortcutPress(int key) {
    #if !defined(EMSCRIPTEN)
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureKeyboard) {
            return false;
        }
    #endif
        return IsKeyPressed(key);
    }

    static inline bool shortcutDown(int key) {
    #if !defined(EMSCRIPTEN)
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureKeyboard) {
            return false;
        }
    #endif
        return IsKeyDown(key);
    }

    static inline bool shortcutReleased(int key) {
    #if !defined(EMSCRIPTEN)
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureKeyboard) {
            return false;
        }
    #endif
        return IsKeyReleased(key);
    }

    static inline bool mousePress(int key) {
    #if !defined(EMSCRIPTEN)
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse) {
            return false;
        }
    #endif
        return IsMouseButtonPressed(key);
    }

    static inline bool mouseDown(int key) {
    #if !defined(EMSCRIPTEN)
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse) {
            return false;
        }
    #endif
        return IsMouseButtonDown(key);
    }

    static inline bool mouseReleased(int key) {
    #if !defined(EMSCRIPTEN)
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse) {
            return false;
        }
    #endif
        return IsMouseButtonReleased(key);
    }

    // Additional IO utility functions can be added here in the future
    // For example:
    // - Mouse handling utilities
    // - File IO helpers
    // - Keyboard state management
    // - Input validation functions
}
