#include "GlobalVars.hpp"
#include <MulNX/Core/Core.hpp>
#include <MulNX/Systems/MessageManager/MessageManager.hpp>
#include <MulNX/Systems/Logger/Logger.hpp>
#include <chrono>

bool MulNX::GlobalVars::Init() {

    this->SendTask("Main", "MulNXMain", [this]()->bool {
        this->Main();
        return true;
        });
    return true;
}

void MulNX::GlobalVars::Main() {
	static std::chrono::steady_clock::time_point StartTime = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point CurrentTime = std::chrono::steady_clock::now();
    static long long LastDelta = 0;
    long long Delta = std::chrono::duration_cast<std::chrono::seconds>(CurrentTime - StartTime).count();
    if (Delta > LastDelta) {
        if (Delta - LastDelta >= 60) {
            this->PublishTickAll();
            this->CoreTick += Delta - LastDelta;
            LastDelta = Delta;
            return;
        }
        for (long long i = 0; i < Delta - LastDelta; ++i) {
            ++this->CoreTick;
            this->Tick();
        }
        LastDelta = Delta;
    }
    return;
}

void MulNX::GlobalVars::PublishTickAll() {
    this->PublishAsync("Core/Tick1"_hash);
    this->PublishAsync("Core/Tick5"_hash);
    this->PublishAsync("Core/Tick10"_hash);
    this->PublishAsync("Core/Tick15"_hash);
    this->PublishAsync("Core/Tick20"_hash);
    this->PublishAsync("Core/Tick30"_hash);
    this->PublishAsync("Core/Tick45"_hash);
    this->PublishAsync("Core/Tick60"_hash);
}
void MulNX::GlobalVars::Tick() {
    this->PublishAsync("Core/Tick1"_hash);
    if (this->CoreTick % 5 == 0) this->PublishAsync("Core/Tick5"_hash);
    if (this->CoreTick % 10 == 0) this->PublishAsync("Core/Tick10"_hash);
    if (this->CoreTick % 15 == 0) this->PublishAsync("Core/Tick15"_hash);
    if (this->CoreTick % 20 == 0) this->PublishAsync("Core/Tick20"_hash);
    if (this->CoreTick % 30 == 0) this->PublishAsync("Core/Tick30"_hash);
    if (this->CoreTick % 45 == 0) this->PublishAsync("Core/Tick45"_hash);
    if (this->CoreTick % 60 == 0) this->PublishAsync("Core/Tick60"_hash);
    if (this->CoreTick % 1800 == 0) this->PublishAsync("Core/Tick30min"_hash);
    return;
}