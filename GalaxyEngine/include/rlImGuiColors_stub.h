#pragma once

#include <imgui.h>
#include <raylib.h>

namespace rlImGuiColors {
    inline Color Convert(const ImVec4& color) {
        return Color{
            static_cast<unsigned char>(ImClamp(color.x, 0.0f, 1.0f) * 255),
            static_cast<unsigned char>(ImClamp(color.y, 0.0f, 1.0f) * 255),
            static_cast<unsigned char>(ImClamp(color.z, 0.0f, 1.0f) * 255),
            static_cast<unsigned char>(ImClamp(color.w, 0.0f, 1.0f) * 255),
        };
    }

    inline ImVec4 Convert(const Color& color) {
        return ImVec4{
            color.r / 255.0f,
            color.g / 255.0f,
            color.b / 255.0f,
            color.a / 255.0f,
        };
    }
}
