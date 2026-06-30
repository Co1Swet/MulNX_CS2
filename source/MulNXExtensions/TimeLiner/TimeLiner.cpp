#include "TimeLiner.hpp"
#include "ITimeAdapter.hpp"
#include <MulNX/Base/UI/UI.hpp>

void TimeLiner::Menu(MulNX::UINode* node) {
    MulNX::UI::RAIIWindow w("时间轴");

    // 没有绑定适配器时，仅绘制静态时间轴
    if (!pTimeAdapter) {
        ImGui::Text("未连接时间适配器");
        return;
    }

    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 win_pos = ImGui::GetWindowPos();
    ImVec2 content_min = ImGui::GetWindowContentRegionMin();
    ImVec2 content_max = ImGui::GetWindowContentRegionMax();

    // 水平线垂直居中
    float line_y = win_pos.y + (content_min.y + content_max.y) * 0.5f;
    float left = win_pos.x + content_min.x + 20.f;
    float right = win_pos.x + content_max.x - 20.f;

    // 从适配器获取范围与当前时间，计算比例
    float minTime = pTimeAdapter->GetMinTime();
    float maxTime = pTimeAdapter->GetMaxTime();
    float curTime = pTimeAdapter->GetTime();

    // 有效范围才更新比例，避免除零
    if (maxTime > minTime) {
        m_clickRatio = (curTime - minTime) / (maxTime - minTime);
        m_clickRatio = std::clamp(m_clickRatio, 0.0f, 1.0f);
    }
    else {
        m_clickRatio = 0.0f;  // 范围无效时不显示指示点
    }

    // 绘制光晕与主线条
    draw->AddLine(ImVec2(left, line_y), ImVec2(right, line_y),
        IM_COL32(255, 255, 255, 40), 10.0f);
    draw->AddLine(ImVec2(left, line_y), ImVec2(right, line_y),
        IM_COL32(180, 180, 180, 220), 6.0f);

    // 绘制当前位置指示点（使用实时比例）
    if (m_clickRatio >= 0.0f && m_clickRatio <= 1.0f && maxTime > minTime) {
        float dot_x = left + m_clickRatio * (right - left);
        draw->AddCircleFilled(ImVec2(dot_x, line_y), 5.0f, IM_COL32(80, 160, 255, 255));
        draw->AddCircle(ImVec2(dot_x, line_y), 5.0f, IM_COL32(255, 255, 255, 100), 12, 2.0f);
    }

    // 透明点击区域
    ImVec2 btn_pos(left, line_y - 12.f);
    ImVec2 btn_size(right - left, 24.f);
    ImGui::SetCursorScreenPos(btn_pos);
    ImGui::InvisibleButton("##timeline_click", btn_size);

    if (ImGui::IsItemClicked() && maxTime > minTime) {
        ImVec2 mouse = ImGui::GetMousePos();
        float ratio = (mouse.x - left) / (right - left);
        ratio = std::clamp(ratio, 0.0f, 1.0f);
        float time = minTime + ratio * (maxTime - minTime);
        if (pTimeAdapter->SetTime(time)) {
            m_clickRatio = ratio;  // 立即更新显示位置
        }
    }

    // 显示当前时间数值
    ImGui::SetCursorScreenPos(ImVec2(left, line_y + 20.f));
    ImGui::Text("时间: %.2f / %.2f", curTime, maxTime);
}

bool TimeLiner::Init() {
    this->SendUIRoot(this->GetName(), [this](MulNX::UINode* node) {return this->Menu(node);});

    return true;
}