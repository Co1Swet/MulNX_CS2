#include "UI.hpp"
#include <format>

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

void MulNX::UI::ShowTime(int tick) {
    int totalSeconds = tick / 64;               // 总秒数（整数）
    int minutes = totalSeconds / 60;            // 分钟
    int secs = totalSeconds % 60;               // 秒
    int subTick = tick % 64;                    // 秒内偏移，范围 0 ~ 63

    ImGui::Text("时间：%d:%02d -- %02d", minutes, secs, subTick);
}

MulNX::UI::RAIIWindow::RAIIWindow(const char* name) {
    this->bNeedCallEnd = true;
    this->bCallBeginResult = ImGui::Begin(name);
}
MulNX::UI::RAIIWindow::RAIIWindow(const char* name, std::atomic<bool>& showWindow) {
    if (!showWindow.load(std::memory_order_acquire))return;
    this->bNeedCallEnd = true;
    bool open = true;
    this->bCallBeginResult = ImGui::Begin(name, &open);
    showWindow.store(open, std::memory_order_release);
}
MulNX::UI::RAIIWindow::~RAIIWindow() {
    if (!this->bNeedCallEnd)return;
    ImGui::End();
    this->bNeedCallEnd = false;
}
MulNX::UI::RAIIWindow::operator bool()const {
    return this->bNeedCallEnd;
}
bool MulNX::UI::RAIIWindow::ShouldDraw()const {
    return this->bCallBeginResult;
}

MulNX::UI::RAIIChild::RAIIChild(const char* str_id, const ImVec2& size_arg, ImGuiChildFlags child_flags, ImGuiWindowFlags window_flags) {
    this->showed = true;
    ImGui::BeginChild(str_id, size_arg, child_flags, window_flags);
}
MulNX::UI::RAIIChild::~RAIIChild() {
    if (this->showed) {
        ImGui::EndChild();
        this->showed = false;
    }
}
MulNX::UI::RAIIChild::operator bool()const {
    return this->showed;
}

MulNX::UI::RAIITable::RAIITable(const char* str_id, const std::vector<std::string>& columns, ImGuiTableFlags flags, const ImVec2& outer_size, float inner_width) {
    this->showed = ImGui::BeginTable(str_id, columns.size(), flags, outer_size, inner_width);
    if (this->showed) {
        for (const auto& column : columns) {
            ImGui::TableSetupColumn(column.c_str());
        }
        ImGui::TableHeadersRow();
    }
}
MulNX::UI::RAIITable::~RAIITable() {
    if (this->showed) {
        ImGui::EndTable();
        this->showed = false;
    }
}
MulNX::UI::RAIITable::operator bool()const {
    return this->showed;
}

bool MulNX::UI::SmartButton::Next(const std::string& label, const ImVec2& size) {
    ++this->counter;
    return ImGui::Button(std::format("{}##{}{}", label, this->counter, this->id).c_str(), size);
}