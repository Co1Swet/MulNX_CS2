#include "ConsoleOutput.hpp"

bool ConsoleOutput::Init() {
    (*this)
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
    case "Log/Error"_hash:
    case "Log/Info"_hash:
    case "Log/Succ"_hash:
    case "Log/Warning"_hash: {
        if(!this->pGlobalVars->SystemReady.load(std::memory_order_relaxed))break;
        auto fmtted = msg.asp.get<MulNX::NetExt>()->str1;
        this->AsyncCommandNoReport(std::format("echoln [MulNX]{}", fmtted));
        break;
    }

    }
}