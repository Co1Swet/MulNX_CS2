#include "TimeController.hpp"


bool TimeController::Init() {

    this->SubscribeSync("Hook/LoadLibraryExW/engine2.dll", [this](MulNX::Message& msg) {
        auto demo = this->CS2->GetDemo();
        this->GetDemoTick = IVClass::Assume(demo)->GetVFunc<int()>(3);
        this->IsPlayingDemo = IVClass::Assume(demo)->GetVFunc<bool()>(11);
        this->IsDemoPaused = IVClass::Assume(demo)->GetVFunc<bool()>(12);
        });

    return true;
}
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