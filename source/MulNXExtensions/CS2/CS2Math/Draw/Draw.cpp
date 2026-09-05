#include "Draw.hpp"
#include <CS2Math/Translate/Translate.hpp>

bool MulNX::UI::DrawWorldLine(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end, const TransInfo& info, ImU32 col, float thickness) {
    DirectX::XMFLOAT2 D21, D22;

    if (!MulNX::Math::WorldToScreen(start, D21, info.pMatrix, info.windowWidth, info.windowHeight))return false;
    if (!MulNX::Math::WorldToScreen(end, D22, info.pMatrix, info.windowWidth, info.windowHeight))return false;

    ImGui::GetBackgroundDrawList()->AddLine({ D21.x,D21.y }, { D22.x,D22.y }, col, thickness);
    return true;
}

bool MulNX::UI::DrawWorldPoint(const DirectX::XMFLOAT3& pos, const TransInfo& info, const char* label) {
    DirectX::XMFLOAT2 D2;
    if (!MulNX::Math::WorldToScreen(pos, D2, info.pMatrix, info.windowWidth, info.windowHeight))return false;
    auto drawList = ImGui::GetBackgroundDrawList();
    drawList->AddCircleFilled({ D2.x,D2.y }, 3.0f, IM_COL32(0, 0, 0, 255));
    if (label) {
        drawList->AddText({ D2.x + 2,D2.y + 2 }, IM_COL32(0, 0, 0, 255), label);
    }
    return true;
}