#include "TimeMarkerManager.hpp"
#include <MulNX/Base/UI/UI.hpp>

void TimeMarkerManager::TimeRend(TimeLiner* timeline, ImDrawList* dl) {
    ImGui::SameLine();
    for (const auto& marker : this->markers) {
        if (marker < timeline->minTime || marker > timeline->maxTime) {
            continue;
        }
        const ImVec2 pos = timeline->Map(marker, 0);
        dl->AddLine(ImVec2(pos.x, pos.y - 10.0f), ImVec2(pos.x, pos.y + 10.0f),
            IM_COL32(255, 190, 60, 220), 2.0f);
    }

    float currentTime = timeline->GetTime();
    if (ImGui::Button("标记当前时间")) {
        this->markers.push_back(currentTime);
    }

    ImGui::SameLine();
    const std::string preview = std::format("{} 个标记", this->markers.size());
    ImGui::SetNextItemWidth(110.0f);
    if (!ImGui::BeginCombo("时间标记", preview.c_str())) {
        return;
    }
    auto endCombo = scope_exit([&]() {
        ImGui::EndCombo();
        });

    if (ImGui::BeginMenu("标记操作")) {
        if (ImGui::MenuItem("按时间排序")) {
            std::stable_sort(this->markers.begin(), this->markers.end());
        }
        if (ImGui::MenuItem("删除所有标记", nullptr, false, !this->markers.empty())) {
            this->markers.clear();
        }
        ImGui::EndMenu();
    }

    if (this->markers.empty()) {
        ImGui::TextDisabled("暂无标记");
        return;
    }

    for (std::size_t i = 0; i < this->markers.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));
        auto popID = scope_exit([&]() {
            ImGui::PopID();
            });
        
        const std::string label = std::format("标记 {}：{:.3f} 秒", i + 1, this->markers[i]);
        const float labelWidth = ImGui::CalcTextSize(label.c_str()).x
            + ImGui::GetStyle().FramePadding.x * 2.0f;
        if (ImGui::Selectable(label.c_str(), false, ImGuiSelectableFlags_None,
            ImVec2(labelWidth, 0.0f))) {
            timeline->SetTime(this->markers[i]);
        }
        ImGui::SameLine();
        if (ImGui::Button("删除")) {
            this->markers.erase(this->markers.begin() + i);
            break;
        }
    }
}

bool TimeMarkerManager::Init() {
    return true;
}