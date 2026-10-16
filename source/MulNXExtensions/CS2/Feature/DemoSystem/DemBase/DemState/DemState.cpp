#include "DemState.hpp"

void DemState::Menu()const {
    for (const auto& [name, demo] : this->demos) {
        if (ImGui::Button(name.c_str())) {
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/SetOperating"_hash);
            rp->str1 = name;
            this->PublishAsync(std::move(msg));
        }
        ImGui::SameLine();
    }
    ImGui::NewLine();
}

bool DemState::Init() {
    (*this)

        ;

    this->UIRegisterCallback("UI.Demo.Choose", [this](auto&&...) {
        this->Menu();
        });
    
    this->SendTask("Update", "DemoSys", [this]() {
        this->Update();
        return true;
        });

    return true;
}

void DemState::ProcessMsg(MulNX::Message& msg) {
    
}