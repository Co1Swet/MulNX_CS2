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
        auto fmtted = std::move(msg.asp.get<MulNX::NetExt>()->str1);
        this->AsyncCommandNoReport("echoln [MulNX]" + fmtted);
        break;
    }

    }
}