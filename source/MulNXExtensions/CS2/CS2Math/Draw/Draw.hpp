#pragma once
#include <DirectXMath.h>
#include <MulNXThirdParty/imgui_d11/imgui.h>

namespace MulNX {
    class TransInfo {
    public:
        float* pMatrix = nullptr;
        int windowHeight = 0;
        int windowWidth = 0;
    };

    namespace UI {
        bool DrawWorldPoint(const DirectX::XMFLOAT3& pos, const TransInfo& info, const char* label);
        bool DrawWorldLine(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end, const TransInfo& info, ImU32 col, float thickness = 1.0f);
    }
}