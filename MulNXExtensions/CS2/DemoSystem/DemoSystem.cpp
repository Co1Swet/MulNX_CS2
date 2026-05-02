#include "DemoSystem.hpp"

bool DemoSystem::Init() {
    this->ISys()
        .SubscribeAsync("Window/Drag/FileDrop");
    
    this->SendTask("MulNXMain", [this]() {
        this->EntryProcessMsg();
        return true;
        });

    return true;
}

void DemoSystem::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Window/Drag/FileDrop"_hash: {
        auto& path = msg.asp.get<MulNX::NetExt>()->str1;
        this->ISys().AsyncCommand(std::format("playdemo \"{}\"", path));
        break;
    }
    }
    
}
