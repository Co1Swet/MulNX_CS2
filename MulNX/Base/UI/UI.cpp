#include "UI.hpp"

bool MulNX::UI::SliderFloat(const char* label, std::atomic<float>& av, float v_min, float v_max, const char* format, ImGuiSliderFlags flags) {
    float v = av.load(std::memory_order_acquire);
    bool changed = ImGui::SliderFloat(label, &v, v_min, v_max, format, flags);
    if (changed) {
        av.store(v, std::memory_order_release);
    }
    return changed;
}
bool MulNX::UI::SliderInt(const char* label, std::atomic<int>& av, int v_min, int v_max, const char* format, ImGuiSliderFlags flags) {
    int v = av.load(std::memory_order_acquire);
    bool changed = ImGui::SliderInt(label, &v, v_min, v_max, format, flags);
    if (changed) {
        av.store(v, std::memory_order_release);
    }
    return changed;
}
bool MulNX::UI::Checkbox(const char* label, std::atomic<bool>& av) {
    bool v = av.load(std::memory_order_acquire);
    bool changed = ImGui::Checkbox(label, &v);
    if (changed) {
        av.store(v, std::memory_order_release);
    }
    return changed;
}
MulNX::UI::RAIIWindow::RAIIWindow(const char* name) {
    this->showed = true;
    ImGui::Begin(name);
}
MulNX::UI::RAIIWindow::RAIIWindow(const char* name, std::atomic<bool>& showWindow) {
    this->showed = showWindow.load(std::memory_order_acquire);
    if (this->showed) {
        bool open = this->showed;
        ImGui::Begin(name, &open);
        showWindow.store(open, std::memory_order_release);
    }
}
MulNX::UI::RAIIWindow::~RAIIWindow() {
    if (this->showed) {
        ImGui::End();
    }   
}
MulNX::UI::RAIIWindow::operator bool()const {
    return this->showed;
}

MulNX::UI::RAIIChild::RAIIChild(const char* str_id, const ImVec2& size_arg, ImGuiChildFlags child_flags, ImGuiWindowFlags window_flags) {
    this->showed = true;
    if (this->showed) {
        bool open = this->showed;
        ImGui::BeginChild(str_id, size_arg, child_flags, window_flags);
    }
}
MulNX::UI::RAIIChild::~RAIIChild() {
    if (this->showed) {
        ImGui::EndChild();
    }
}
MulNX::UI::RAIIChild::operator bool()const {
    return this->showed;
}

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
    drawList->AddCircleFilled({D2.x,D2.y}, 3.0f, IM_COL32(0, 0, 0, 255));
    if (label) {
        drawList->AddText({ D2.x + 2,D2.y + 2 }, IM_COL32(0, 0, 0, 255), label);
    }
    return true;
}