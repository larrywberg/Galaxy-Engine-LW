#include "pch.h"

#if defined(EMSCRIPTEN)

#include <rlgl.h>

namespace {
static Texture2D g_fontTexture = { 0 };
static bool g_initialized = false;

void EnsureContext() {
    if (ImGui::GetCurrentContext() == nullptr) {
        ImGui::CreateContext();
    }
}

void UpdateFonts() {
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels = nullptr;
    int width = 0;
    int height = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    if (g_fontTexture.id != 0) {
        UnloadTexture(g_fontTexture);
        g_fontTexture = { 0 };
    }

    Image image = { 0 };
    image.data = pixels;
    image.width = width;
    image.height = height;
    image.mipmaps = 1;
    image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;

    g_fontTexture = LoadTextureFromImage(image);
    io.Fonts->SetTexID((ImTextureID)(uintptr_t)g_fontTexture.id);
}

void RenderDrawData(ImDrawData* draw_data) {
    if (draw_data == nullptr || draw_data->TotalVtxCount == 0) {
        return;
    }

    const ImVec2 clip_off = draw_data->DisplayPos;
    const ImVec2 clip_scale = draw_data->FramebufferScale;

    const float display_w = draw_data->DisplaySize.x * clip_scale.x;
    const float display_h = draw_data->DisplaySize.y * clip_scale.y;
    if (display_w <= 0.0f || display_h <= 0.0f) {
        return;
    }

    rlDrawRenderBatchActive();

    rlMatrixMode(RL_PROJECTION);
    rlPushMatrix();
    rlLoadIdentity();
    rlOrtho(0.0f, draw_data->DisplaySize.x, draw_data->DisplaySize.y, 0.0f, -1.0f, 1.0f);

    rlMatrixMode(RL_MODELVIEW);
    rlPushMatrix();
    rlLoadIdentity();

    rlEnableScissorTest();
    rlSetBlendMode(BLEND_ALPHA);

    for (int n = 0; n < draw_data->CmdListsCount; n++) {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawVert* vtx_buffer = cmd_list->VtxBuffer.Data;
        const ImDrawIdx* idx_buffer = cmd_list->IdxBuffer.Data;

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->ElemCount == 0) {
                continue;
            }

            ImVec4 clip_rect;
            clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
            clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
            clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
            clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

            if (clip_rect.x >= display_w || clip_rect.y >= display_h || clip_rect.z <= 0.0f || clip_rect.w <= 0.0f) {
                idx_buffer += pcmd->ElemCount;
                continue;
            }

            BeginScissorMode(
                static_cast<int>(clip_rect.x),
                static_cast<int>(clip_rect.y),
                static_cast<int>(clip_rect.z - clip_rect.x),
                static_cast<int>(clip_rect.w - clip_rect.y)
            );

            const unsigned int tex_id = pcmd->TextureId ? static_cast<unsigned int>(static_cast<uintptr_t>(pcmd->TextureId)) : 0;
            rlSetTexture(tex_id);

            rlBegin(RL_TRIANGLES);
            for (unsigned int i = 0; i < pcmd->ElemCount; i++) {
                const ImDrawIdx idx = idx_buffer[i];
                const ImDrawVert& v = vtx_buffer[idx];
                const unsigned int col = v.col;
                const unsigned char r = static_cast<unsigned char>(col & 0xFF);
                const unsigned char g = static_cast<unsigned char>((col >> 8) & 0xFF);
                const unsigned char b = static_cast<unsigned char>((col >> 16) & 0xFF);
                const unsigned char a = static_cast<unsigned char>((col >> 24) & 0xFF);

                rlColor4ub(r, g, b, a);
                rlTexCoord2f(v.uv.x, v.uv.y);
                rlVertex2f(v.pos.x, v.pos.y);
            }
            rlEnd();

            rlSetTexture(0);
            EndScissorMode();

            idx_buffer += pcmd->ElemCount;
        }
    }

    rlDisableScissorTest();

    rlMatrixMode(RL_MODELVIEW);
    rlPopMatrix();
    rlMatrixMode(RL_PROJECTION);
    rlPopMatrix();
}
} // namespace

void rlImGuiSetup(bool darkStyle) {
    EnsureContext();

    ImGuiIO& io = ImGui::GetIO();
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;

    if (darkStyle) {
        ImGui::StyleColorsDark();
    } else {
        ImGui::StyleColorsClassic();
    }

    UpdateFonts();
    g_initialized = true;
}

void rlImGuiBegin() {
    EnsureContext();

    ImGuiIO& io = ImGui::GetIO();
    if (io.Fonts->TexID == 0 || g_fontTexture.id == 0) {
        UpdateFonts();
    }
    io.DeltaTime = GetFrameTime();

    float width = static_cast<float>(GetScreenWidth());
    float height = static_cast<float>(GetScreenHeight());
    if (width <= 0.0f) {
        width = 1.0f;
    }
    if (height <= 0.0f) {
        height = 1.0f;
    }
    io.DisplaySize = ImVec2(width, height);

    Vector2 mouse = GetMousePosition();
    io.MousePos = ImVec2(mouse.x, mouse.y);
    io.MouseDown[0] = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    io.MouseDown[1] = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
    io.MouseDown[2] = IsMouseButtonDown(MOUSE_BUTTON_MIDDLE);
    io.MouseWheel = GetMouseWheelMove();

    io.AddKeyEvent(ImGuiKey_LeftCtrl, IsKeyDown(KEY_LEFT_CONTROL));
    io.AddKeyEvent(ImGuiKey_RightCtrl, IsKeyDown(KEY_RIGHT_CONTROL));
    io.AddKeyEvent(ImGuiKey_LeftShift, IsKeyDown(KEY_LEFT_SHIFT));
    io.AddKeyEvent(ImGuiKey_RightShift, IsKeyDown(KEY_RIGHT_SHIFT));
    io.AddKeyEvent(ImGuiKey_LeftAlt, IsKeyDown(KEY_LEFT_ALT));
    io.AddKeyEvent(ImGuiKey_RightAlt, IsKeyDown(KEY_RIGHT_ALT));
    io.AddKeyEvent(ImGuiKey_LeftSuper, IsKeyDown(KEY_LEFT_SUPER));
    io.AddKeyEvent(ImGuiKey_RightSuper, IsKeyDown(KEY_RIGHT_SUPER));

    io.AddKeyEvent(ImGuiKey_Tab, IsKeyDown(KEY_TAB));
    io.AddKeyEvent(ImGuiKey_LeftArrow, IsKeyDown(KEY_LEFT));
    io.AddKeyEvent(ImGuiKey_RightArrow, IsKeyDown(KEY_RIGHT));
    io.AddKeyEvent(ImGuiKey_UpArrow, IsKeyDown(KEY_UP));
    io.AddKeyEvent(ImGuiKey_DownArrow, IsKeyDown(KEY_DOWN));
    io.AddKeyEvent(ImGuiKey_PageUp, IsKeyDown(KEY_PAGE_UP));
    io.AddKeyEvent(ImGuiKey_PageDown, IsKeyDown(KEY_PAGE_DOWN));
    io.AddKeyEvent(ImGuiKey_Home, IsKeyDown(KEY_HOME));
    io.AddKeyEvent(ImGuiKey_End, IsKeyDown(KEY_END));
    io.AddKeyEvent(ImGuiKey_Insert, IsKeyDown(KEY_INSERT));
    io.AddKeyEvent(ImGuiKey_Delete, IsKeyDown(KEY_DELETE));
    io.AddKeyEvent(ImGuiKey_Backspace, IsKeyDown(KEY_BACKSPACE));
    io.AddKeyEvent(ImGuiKey_Space, IsKeyDown(KEY_SPACE));
    io.AddKeyEvent(ImGuiKey_Enter, IsKeyDown(KEY_ENTER));
    io.AddKeyEvent(ImGuiKey_Escape, IsKeyDown(KEY_ESCAPE));

    io.AddKeyEvent(ImGuiKey_A, IsKeyDown(KEY_A));
    io.AddKeyEvent(ImGuiKey_C, IsKeyDown(KEY_C));
    io.AddKeyEvent(ImGuiKey_V, IsKeyDown(KEY_V));
    io.AddKeyEvent(ImGuiKey_X, IsKeyDown(KEY_X));
    io.AddKeyEvent(ImGuiKey_Y, IsKeyDown(KEY_Y));
    io.AddKeyEvent(ImGuiKey_Z, IsKeyDown(KEY_Z));

    unsigned int codepoint = 0;
    while ((codepoint = GetCharPressed()) != 0) {
        io.AddInputCharacter(static_cast<unsigned int>(codepoint));
    }

    ImGui::NewFrame();
}

void rlImGuiEnd() {
    if (ImGui::GetCurrentContext() == nullptr) {
        return;
    }

    ImGui::Render();
    RenderDrawData(ImGui::GetDrawData());
}

void rlImGuiShutdown() {
    if (g_fontTexture.id != 0) {
        UnloadTexture(g_fontTexture);
        g_fontTexture = { 0 };
    }

    if (ImGui::GetCurrentContext() != nullptr) {
        ImGui::DestroyContext();
    }
    g_initialized = false;
}

#endif
