#include "TimeController.hpp"
#include <MulNXExtensions/TimeLiner/TimeLiner.hpp>
#include <Mirror/CSDemoController/CSDemoController.hpp>

void TimeController::CameraTimeMenu() {
    if (MulNX::UI::Checkbox("subtick时间轴", this->subtickCameraPath)) {
        this->cameraTimeGeneration.fetch_add(1);
        this->cameraTimeStatus.store(this->IsSubtickCameraPathEnabled()
            ? MulNX::CameraTimeStatus::Calibrating : MulNX::CameraTimeStatus::Legacy);
    }
    ImGui::TextWrapped("实验性功能：改善高 FPS、慢动作 Demo 下 Camera Path 的连续性。仅作用于运镜，独立于视角平滑；不支持的构建或无效相位会回退整数 Tick。");
    if (this->IsSubtickCameraPathEnabled())
        ImGui::TextDisabled("%s", MulNX::CameraTimeStatusName(this->cameraTimeStatus.load()));
}
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
                this->AsyncCommand(std::format("host_timescale {:.2f}", kSpeeds[i]));
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
}

bool TimeController::Init() {
    this->pTimeline->pTimeAdapter1 = this;
    this->pCSDemoController = this->FindModule<CSDemoController>("CSDemoController");
    this->SubscribeAsync("Demo/GotoTick/Complete").SubscribeAsync("Demo/SetPlaying");
    this->SubscribeSync("Hook/CSMainLoop", [this](auto&&...) { this->Update(); });
    this->SubscribeSync("Hook/MainLoop/TickJumpDetected", [this](auto&&...) { this->cameraTimeGeneration.fetch_add(1); });
    this->UIRegisterCallback("UI.CameraSetting.SubtickTime", [this](auto&&...) { this->CameraTimeMenu(); });

    this->SubscribeSync("Hook/LoadLibraryExW/engine2.dll", [this](MulNX::Message& msg) {
        auto demo = this->CS2->GetDemo();
        this->pDemoPlayer = demo;
        this->GetDemoTick = IVClass::Assume(demo)->GetVFunc<int()>(3);
        this->IsPlayingDemo = IVClass::Assume(demo)->GetVFunc<bool()>(11);
        this->IsDemoPaused = IVClass::Assume(demo)->GetVFunc<bool()>(12);
        });

    return true;
}
void TimeController::ProcessMsg(MulNX::Message& msg) {
    if (msg.type == "Demo/GotoTick/Complete"_hash || msg.type == "Demo/SetPlaying"_hash)
        this->cameraTimeGeneration.fetch_add(1);
}
// 时间适配器接口实现
float TimeController::GetMinTime() {
    return 0;
}
float TimeController::GetMaxTime() {
    return this->GetDemoLastTick() * (1.0f / 64.0f);
}
int TimeController::GetDemoLastTick()const {
    return MulNX::MRead((int*)((uintptr_t)this->pDemoPlayer + 0x12C));
}
float TimeController::GetTime() {
    auto tick = this->GetDemoTick();
    return tick * (1.0f / 64.0f);
}
MulNX::CameraPathTime TimeController::GetCameraPathTime() {
    if (!this->IsSubtickCameraPathEnabled()) return ITimeAdapter::GetCameraPathTime();
    const int tick = this->GetDemoTick();
    const auto clientBase = this->CS2->client.GetBaseAddress();
    const auto engineBase = this->CS2->engine2.GetBaseAddress();
    auto input = MulNX::ExperimentalCameraTime::ReadProbe(clientBase, engineBase,
        clientBase + cs2_dumper::offsets::client_dll::dwGlobalVars,
        reinterpret_cast<uintptr_t>(this->pDemoPlayer), tick, this->cameraTimeGeneration.load());
    if (input.generation != this->cameraTimeGeneration.load())
        input.validity = MulNX::CameraTimeStatus::UnstableSnapshot;
    std::unique_lock lock(this->cameraTimeMutex, std::try_to_lock);
    MulNX::CameraPathTime sample;
    if (lock.owns_lock()) sample = this->cameraTimeSampler.Sample(input);
    else {
        sample.time = double(tick) / 64.0;
        sample.requested = true; sample.status = MulNX::CameraTimeStatus::Busy;
    }
    this->cameraTimeStatus.store(sample.status);
    return sample;
}
bool TimeController::SetTime(float time) {
    this->cameraTimeGeneration.fetch_add(1);
    this->AsyncCommand(std::format("demo_gototick {}", static_cast<int>(time * 64.0f)));
    return true;
}
bool TimeController::JumpRealRel(float time) {
    return this->SetTime(time + this->GetTime());
}
