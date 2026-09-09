#include "TimeController.hpp"
#include <MulNXExtensions/TimeLiner/TimeLiner.hpp>

bool TimeController::Init() {
    this->FindModule<TimeLiner>("TimeLiner")->pTimeAdapter1 = this;

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
// 可被调用接口实现
float TimeController::GetReal() {
    auto time = this->GetDemoTick() / 64.0f;
    return time;
}
bool TimeController::JumpReal(const float time) {
    int currentGameTick = this->GetReal() * 64;
    int currentDemoTick = this->GetDemoTick();

    int targetGameTick = static_cast<int>(time * 64);
    int deltaTick = currentGameTick - currentDemoTick;
    int tick = targetGameTick - deltaTick;

    std::string command = std::format("demo_gototick {}", tick);
    this->AsyncCommand(std::move(command));
    return true;
}
bool TimeController::JumpRealRel(float time) {
    return this->JumpReal(time + this->GetReal());
}