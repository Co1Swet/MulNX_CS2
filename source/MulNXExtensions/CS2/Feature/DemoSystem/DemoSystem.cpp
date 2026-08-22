#include "DemoSystem.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <Buildup/TimeController/TimeController.hpp>

void DemoSystem::Window(MulNX::UICoordinator* uico) {
    auto w = MulNX::UI::RAIIWindow("Demo", this->showWindow);
    if (!w) return;
    uico->CallbackCall("UI.Demos"_hash, nullptr);
    if (!w.ShouldDraw())return;

    uico->CallbackCall("UI.Demo.Main"_hash, nullptr);

    ImGui::Separator();

    ImGui::Text(I18n("demo.status.is_playing", this->CS2Time->IsPlayingDemo()).c_str());
    ImGui::Text(I18n("demo.status.is_pausing", this->CS2Time->IsDemoPaused()).c_str());

    ImGui::Separator();
}

bool DemoSystem::Init() {
    (*this)
        .SubscribeAsync("Demo/Play")
        .SubscribeAsync("Window/Drag/FileDrop")
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
    case "Window/Drag/FileDrop"_hash: {
        auto& path = msg.asp.get<MulNX::NetExt>()->str1;
        std::filesystem::path file = path;
        auto ext = file.extension();
        if (ext != ".dem")break;
        try {
            std::filesystem::copy(file, this->CS2Paths->demo / file.filename(), std::filesystem::copy_options::overwrite_existing);
        }
        catch (const std::filesystem::filesystem_error& e) {
            this->LogError(I18n("demo.copy_failed", e.what()).c_str());
        }
        this->PublishAsync("Demo/Refresh"_hash);
        break;
    }
    case "Demo/Play"_hash: {
        auto& path = msg.asp.get<MulNX::NetExt>()->str1;
        this->AsyncCommand(std::format("playdemo \"{}\"", path));
        break;
    }
    }
}