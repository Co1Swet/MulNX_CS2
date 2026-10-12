#include "DemoSystem.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <Support/TimeController/TimeController.hpp>
#include <shellapi.h>

void DemoSystem::Window(MulNX::UICoordinator* uico) {
    auto w = MulNX::UI::RAIIWindow("Demo系统", this->showWindow);
    if (!w) return;
    uico->CallbackCall("UI.Demos"_hash, nullptr);
    if (!w.ShouldDraw())return;
    uico->CallbackCall("UI.Demo.Main"_hash, nullptr);
}

bool DemoSystem::Init() {
    (*this)
        .SubscribeAsync("Demo/Play")
        ;

    this->UIRegisterCallback("UI.Advanced", [this](auto&&...) {
        MulNX::UI::Checkbox("Demo系统", this->showWindow);
        });

    this->SendUIRoot(this->GetName(), [this](auto uico, auto&&...) {
        return this->Window(uico);
        });

    this->SendTask("Update", "DemoSys", [this]() {
        this->Update();
        return true;
        });

    this->PublishAsync("Demo/Refresh"_hash);

    return true;
}

void DemoSystem::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Demo/Play"_hash: {
        auto& path = msg.asp.get<MulNX::NetExt>()->str1;
        this->AsyncCommand(std::format("playdemo \"{}\"", path));
        break;
    }
    }
}