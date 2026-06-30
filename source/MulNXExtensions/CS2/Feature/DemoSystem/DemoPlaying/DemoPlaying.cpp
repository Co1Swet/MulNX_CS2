#include "DemoPlaying.hpp"
#include <MulNXExtensions/TimeLiner/TimeLiner.hpp>

bool DemoPlaying::Init() {
    this->FindModule<TimeLiner>("TimeLiner")->pTimeAdapter = this;

    this->SubscribeSync("Hook/LoadLibraryExW/engine2.dll", [this](MulNX::Message& msg) {
        auto demo = this->CS2->GetDemo();
        this->GetDemoTick = IVClass::Assume(demo)->GetVFunc<int()>(3);
        this->IsPlayingDemo = IVClass::Assume(demo)->GetVFunc<bool()>(11);
        this->IsDemoPaused = IVClass::Assume(demo)->GetVFunc<bool()>(12);

        this->pDemoPlayer = demo;
        });

    return true;
}

float DemoPlaying::GetMinTime() {

    return 0;
}
float DemoPlaying::GetMaxTime() {
    // 读取 CDemoPlayer 偏移 +0x200 处的总 tick（int32）
    int totalTicks = *(int*)((uintptr_t)this->pDemoPlayer + 0x12C);
    // 转换为秒
    return totalTicks * (1.0f / 64.0f);
}

float DemoPlaying::GetTime() {
    auto tick = this->GetDemoTick();
    return tick * (1.0f / 64.0f);
}
bool DemoPlaying::SetTime(float time) {
    this->AsyncCommand(std::format("demo_gototick {}", static_cast<int>(time) * 64));
    return true;
}