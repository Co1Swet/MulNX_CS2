#pragma once

#include <MulNXThirdParty/imgui_d11/imgui.h>
#include <MulNXThirdParty/imgui_d11/imgui_stdlib.h>
#include <atomic>
#include <MulNX/Base/Math/Math.hpp>

namespace MulNX {
    class TransInfo {
    public:
        float* pMatrix = nullptr;
        int windowHeight = 0;
        int windowWidth = 0;
    };

    namespace UI {
        bool SliderFloat(const char* label, std::atomic<float>& av, float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
        bool SliderInt(const char* label, std::atomic<int>& av, int v_min, int v_max, const char* format = "%d", ImGuiSliderFlags flags = 0);
        bool Checkbox(const char* label, std::atomic<bool>& av);

        bool DrawWorldPoint(const DirectX::XMFLOAT3& pos, const TransInfo& info, const char* label);
        bool DrawWorldLine(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end, const TransInfo& info, ImU32 col, float thickness = 1.0f);
        class RAIIWindow {
            bool showed;
        public:
            RAIIWindow() = delete;
            RAIIWindow(const char* name);
            RAIIWindow(const char* name, std::atomic<bool>& showWindow);
            ~RAIIWindow();
            explicit operator bool() const;
        };

        class RAIIChild {
            bool showed;
        public:
            RAIIChild() = delete;
            RAIIChild(const char* str_id, const ImVec2& size = ImVec2(0, 0), ImGuiChildFlags child_flags = 0, ImGuiWindowFlags window_flags = 0);
            ~RAIIChild();
            explicit operator bool() const;
        };
    }
}