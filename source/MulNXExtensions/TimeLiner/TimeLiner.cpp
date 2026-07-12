#include "TimeLiner.hpp"
#include "ITimeAdapter.hpp"
#include "TimeLineModuleBase.hpp"
#include <MulNX/Base/UI/UI.hpp>

void TimeLiner::Menu() {
    MulNX::UI::RAIIWindow w("时间轴");

    if (!pTimeAdapter) {
        ImGui::Text("未连接时间适配器");
        return;
    }

    this->UpdateTime();
    this->UpdatePos();

    ImDrawList* draw = ImGui::GetWindowDrawList();

    // 有效范围才更新比例，避免除零
    if (maxTime > minTime) {
        m_clickRatio = (curTime - minTime) / (maxTime - minTime);
        m_clickRatio = std::clamp(m_clickRatio, 0.0f, 1.0f);
    }
    else {
        m_clickRatio = 0.0f;  // 范围无效时不显示指示点
    }

    // 绘制光晕与主线条
    draw->AddLine(ImVec2(this->currentLeftX, this->currentBaseY), ImVec2(this->currentRightX, this->currentBaseY),
        IM_COL32(255, 255, 255, 40), 10.0f);
    draw->AddLine(ImVec2(this->currentLeftX, this->currentBaseY), ImVec2(this->currentRightX, this->currentBaseY),
        IM_COL32(180, 180, 180, 220), 6.0f);

    // 绘制当前位置指示点（使用实时比例）
    if (m_clickRatio >= 0.0f && m_clickRatio <= 1.0f && maxTime > minTime) {
        float dot_x = this->currentLeftX + m_clickRatio * (this->currentRightX - this->currentLeftX);
        draw->AddCircleFilled(ImVec2(dot_x, this->currentBaseY), 5.0f, IM_COL32(80, 160, 255, 255));
        draw->AddCircle(ImVec2(dot_x, this->currentBaseY), 5.0f, IM_COL32(255, 255, 255, 100), 12, 2.0f);
    }

    // 透明点击区域
    ImVec2 btn_pos(this->currentLeftX, this->currentBaseY - 12.f);
    ImVec2 btn_size(this->currentRightX - this->currentLeftX, 24.f);
    ImGui::SetCursorScreenPos(btn_pos);
    ImGui::InvisibleButton("##timeline_click", btn_size);

    if (ImGui::IsItemClicked() && maxTime > minTime) {
        ImVec2 mouse = ImGui::GetMousePos();
        float ratio = (mouse.x - this->currentLeftX) / (this->currentRightX - this->currentLeftX);
        ratio = std::clamp(ratio, 0.0f, 1.0f);
        float time = minTime + ratio * (maxTime - minTime);
        if (pTimeAdapter->SetTime(time)) {
            m_clickRatio = ratio;  // 立即更新显示位置
        }
    }

    // 显示当前时间数值
    ImGui::SetCursorScreenPos(ImVec2(this->currentLeftX, this->currentBaseY + 20.f));
    ImGui::Text("时间: %.2f / %.2f", curTime, maxTime);

    for (auto* pModule : this->timeLineModules) {
        ImGui::PushID(pModule);
        pModule->TimeLineCallback(this, draw);
        ImGui::PopID();
    }
}

bool TimeLiner::Init() {
    this->SendUIRoot(this->GetName(), [this](auto&&...) {return this->Menu();});

    return true;
}

void TimeLiner::UpdateTime() {
    // 从适配器获取范围与当前时间，计算比例
    this->minTime = pTimeAdapter->GetMinTime();
    this->maxTime = pTimeAdapter->GetMaxTime();
    this->curTime = pTimeAdapter->GetTime();
}

void TimeLiner::UpdatePos() {
    ImVec2 win_pos = ImGui::GetWindowPos();
    ImVec2 content_min = ImGui::GetWindowContentRegionMin();
    ImVec2 content_max = ImGui::GetWindowContentRegionMax();

    // ---- 更新缓存 ----
    this->currentBaseY = win_pos.y + (content_min.y + content_max.y) * 0.5f;
    this->currentLeftX = win_pos.x + content_min.x + 20.f;
    this->currentRightX = win_pos.x + content_max.x - 20.f;
}

ImVec2 TimeLiner::Map(float time, int layer) const {
    // 防止除零（虽然调用前会保证 max > min，但依旧防御）
    float range = this->maxTime - this->minTime;
    if (range <= 0.0f) {
        return ImVec2(this->currentLeftX, this->currentBaseY);
    }
    float ratio = (time - this->minTime) / range;
    float x = this->currentLeftX + ratio * (this->currentRightX - this->currentLeftX);
    float y = this->currentBaseY - layer * 20.0f; // 层高固定为20像素
    return ImVec2(x, y);
}