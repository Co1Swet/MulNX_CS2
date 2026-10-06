#include "TimeController.hpp"
#include <MulNXExtensions/TimeLiner/TimeLiner.hpp>
#include <Mirror/CSDemoController/CSDemoController.hpp>

void TimeController::TimeRend(TimeLiner* timeline, ImDrawList* dl) {
    ImGui::SameLine();
    // 状态快照
    const bool paused = this->IsDemoPaused();
    const int  currentRound = this->pCSDemoController->GetCurrentRound();
    const int  totalRounds = this->pCSDemoController->GetTotalRound();
    const bool canNext = currentRound >= 0 && currentRound < totalRounds;

    // 播放 / 暂停
    if (ImGui::Button(paused ? "▶ 播放" : "⏸ 暂停")) {
        this->AsyncCommand(paused ? "demo_resume" : "demo_pause");
    }

    // 上一回合
    ImGui::SameLine();
    if (ImGui::Button("◀ 上一回合")) {
        this->AsyncCommand("MulNX/Demo/PrevRound");
    }

    // 当前回合显示
    ImGui::SameLine();
    ImGui::Text("回合 %d / %d", currentRound, totalRounds);

    // 下一回合
    ImGui::SameLine();
    if (ImGui::Button("下一回合 ▶")) {
        this->AsyncCommand("MulNX/Demo/NextRound");
    }

    // 倍速循环
    ImGui::SameLine();
    static const float kSpeeds[] = { 0.25f, 0.5f, 1.0f, 2.0f, 4.0f };
    static int sSpeedIdx = 2;
    if (ImGui::Button(std::format("{:.2f}x", kSpeeds[sSpeedIdx]).c_str())) {
        sSpeedIdx = (sSpeedIdx + 1) % 5;
        this->AsyncCommand(std::format("demo_timescale {:.2f}", kSpeeds[sSpeedIdx]));
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