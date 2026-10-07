#include "TimeController.hpp"
#include <MulNXExtensions/TimeLiner/TimeLiner.hpp>
#include <Mirror/CSDemoController/CSDemoController.hpp>

void TimeController::TimeRend(TimeLiner* timeline, ImDrawList* dl) {
    ImGui::SameLine();
    // 状态快照
    const bool paused = this->IsDemoPaused();
    const int  currentRound = this->pCSDemoController->GetCurrentRound();
    const int  totalRounds = this->pCSDemoController->GetTotalRound();

    if (ImGui::Button(paused ? "▶ 播放" : "⏸ 暂停")) {
        this->AsyncCommand(paused ? "demo_resume" : "demo_pause");
    }

    static constexpr float kSteps[] = { -15.0f, -5.0f, -1.0f, 1.0f, 5.0f, 15.0f };
    static constexpr const char* kLabels[] = { "-15s", "-5s", "-1s", "+1s", "+5s", "+15s" };

    const float currentTime = this->GetTime();
    const float maxTime = this->GetMaxTime();

    for (int i = 0; i < 6; ++i) {
        if (i > 0) ImGui::SameLine();

        const float target = currentTime + kSteps[i];
        const bool  valid = target >= 0.0f && target <= maxTime;

        ImGui::SameLine();
        ImGui::BeginDisabled(!valid);
        if (ImGui::Button(kLabels[i])) {
            this->JumpRealRel(kSteps[i]);
        }
        ImGui::EndDisabled();
    }

    ImGui::SameLine();
    if (ImGui::Button("◀ 上一回合")) {
        this->AsyncCommand("MulNX/Demo/PrevRound");
    }

    ImGui::SameLine();
    ImGui::Text("回合 %d / %d", currentRound, totalRounds);

    ImGui::SameLine();
    if (ImGui::Button("下一回合 ▶")) {
        this->AsyncCommand("MulNX/Demo/NextRound");
    }

    ImGui::SameLine();
    static const float kSpeeds[] = { 0.25f, 0.5f, 1.0f, 2.0f, 4.0f };
    static constexpr int kSpeedCount = IM_ARRAYSIZE(kSpeeds);
    static int sSpeedIdx = 2;

    ImGui::SetNextItemWidth(80.0f);
    if (ImGui::BeginCombo("##speed", std::format("{:.2f}x", kSpeeds[sSpeedIdx]).c_str())) {
        for (int i = 0; i < kSpeedCount; ++i) {
            const bool selected = (i == sSpeedIdx);
            if (ImGui::Selectable(std::format("{:.2f}x", kSpeeds[i]).c_str(), selected)) {
                sSpeedIdx = i;
                this->AsyncCommand(std::format("demo_timescale {:.2f}", kSpeeds[i]));
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
}

bool TimeController::Init() {
    this->FindModule<TimeLiner>("TimeLiner")->pTimeAdapter1 = this;
    this->pCSDemoController = this->FindModule<CSDemoController>("CSDemoController");

    this->SubscribeSync("Hook/LoadLibraryExW/engine2.dll", [this](MulNX::Message& msg) {
        auto demo = this->CS2->GetDemo();
        this->pDemoPlayer = demo;
        this->GetDemoTick = IVClass::Assume(demo)->GetVFunc<int()>(3);
        this->IsPlayingDemo = IVClass::Assume(demo)->GetVFunc<bool()>(11);
        this->IsDemoPaused = IVClass::Assume(demo)->GetVFunc<bool()>(12);
        });

    return true;
}
// 时间适配器接口实现
float TimeController::GetMinTime() {
    return 0;
}
float TimeController::GetMaxTime() {
    // 读取 CDemoPlayer 偏移 +0x200 处的总 tick（int32）
    int totalTicks = *(int*)((uintptr_t)this->pDemoPlayer + 0x12C);
    // 转换为秒
    return totalTicks * (1.0f / 64.0f);
}
float TimeController::GetTime() {
    auto tick = this->GetDemoTick();
    return tick * (1.0f / 64.0f);
}
bool TimeController::SetTime(float time) {
    this->AsyncCommand(std::format("demo_gototick {}", static_cast<int>(time) * 64));
    return true;
}
bool TimeController::JumpRealRel(float time) {
    return this->SetTime(time + this->GetTime());
}