#include "ConsoleOutput.hpp"

bool ConsoleOutput::Init() {
    (*this)
        .SubscribeAsync<void>("ConsoleOutput/Enable")
        .SubscribeAsync<void>("ConsoleOutput/Disable")
        .SubscribeAsync("Log/Info")
        .SubscribeAsync("Log/Succ")
        .SubscribeAsync("Log/Warning")
        .SubscribeAsync("Log/Error")
        ;

    this->SendTask("ConsoleOutput", "MulNXMain", [this]() {
        this->Update();
        return true;
        });
    
    return true;
}

void ConsoleOutput::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "ConsoleOutput/Enable"_hash: {
        this->enable.store(true, std::memory_order_release);
        break;
    }
    case "ConsoleOutput/Disable"_hash: {
        this->enable.store(false, std::memory_order_release);
        break;
    }
    case "Log/Error"_hash:
    case "Log/Info"_hash:
    case "Log/Succ"_hash:
    case "Log/Warning"_hash: {
        if(!this->enable.load(std::memory_order_acquire))break;
        auto fmtted = msg.asp.get<MulNX::NetExt>()->str1;
        this->AsyncCommandNoReport(std::format("echoln [MulNX]{}", fmtted));
        break;
    }
    }
}