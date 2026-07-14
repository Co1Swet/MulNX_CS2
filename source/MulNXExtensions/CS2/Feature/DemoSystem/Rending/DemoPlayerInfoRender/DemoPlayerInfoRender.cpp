#include "DemoPlayerInfoRender.hpp"
#include <MulNX/Base/UI/UI.hpp>

void DemoPlayerInfoRender::Menu(MulNX::Message* umsg) {
    auto&& [uid] = umsg->Access<Steam64UID>();
    this->Update();
    auto it = this->crosshairShareCodes.find(uid);
    if (it == this->crosshairShareCodes.end()) {
        ImGui::Text("无准星信息");
        return;
    }
    std::string& code = it->second;

    ImGui::InputText("##sharecode", code.data(), code.size() + 1,
        ImGuiInputTextFlags_ReadOnly);
    ImGui::SameLine();
    if (ImGui::Button("复制到剪贴板")) {
        ImGui::SetClipboardText(code.c_str());
    }
}


bool DemoPlayerInfoRender::Init() {
    this->SubscribeAsync("Demo/InfoLoad");

    this->UIRegisterCallback("UI.Player.Info", [this](auto uico, auto umsg) {this->Menu(umsg);});

    return true;
}

void DemoPlayerInfoRender::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Demo/InfoLoad"_hash: {
        auto pInfo = msg.asp.get<Demo::Info>();
        for (const auto& [uid, player] : pInfo->players) {
            this->crosshairShareCodes[uid] = player.crosshairShareCode;
        }
        break;
    }
    default:break;
    }
}