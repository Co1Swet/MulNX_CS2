#include "DemoPlayerInfoRender.hpp"
#include <MulNX/Base/UI/UI.hpp>

void DemoPlayerInfoRender::Menu(MulNX::Message* umsg) {
    auto&& [uid] = umsg->Access<Steam64UID>();
    this->Update();
    auto it = this->crosshairShareCodes.find(uid);
    if (it != this->crosshairShareCodes.end()) {
        std::string& code = it->second;

        ImGui::InputText("Demo准星", code.data(), code.size() + 1,
            ImGuiInputTextFlags_ReadOnly);
        ImGui::SameLine();
        if (ImGui::Button("复制到剪贴板")) {
            ImGui::SetClipboardText(code.c_str());
        }
    }
    else {
        ImGui::Text("无Demo准星信息");
    }
    

    auto pPawn = this->CS2Entitys->TryGetObservingPawn();
    if (!pPawn)return;
    auto pCtrler = this->CS2Entitys->GetBaseEntityFromHandle(MulNX::MRead(pPawn->m_hController()))
        ->As<CS2::CCSPlayerController>();
    if (!pCtrler)return;
    auto Symbol = MulNX::MRead(pCtrler->m_szCrosshairCodes());

    auto str = MulNX::Memory::ReadString(Symbol.pStr).value_or("读取失败");

    ImGui::InputText("内存准星", str.data(), str.size() + 1,
        ImGuiInputTextFlags_ReadOnly);
    ImGui::SameLine();
    if (ImGui::Button("复制内存准星到剪贴板")) {
        ImGui::SetClipboardText(str.c_str());
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