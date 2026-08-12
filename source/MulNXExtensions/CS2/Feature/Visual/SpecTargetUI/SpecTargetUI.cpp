#include "SpecTargetUI.hpp"
#include <MulNX/Base/UI/UI.hpp>

void SpecTargetUI::Window(MulNX::UICoordinator* uico) {
    auto w = MulNX::UI::RAIIWindow("观战信息");
    if (!w)return;
    auto pOBing = this->CS2->client.TryGetObservingPawn();
    if (!pOBing) {
        ImGui::Text("当前未观战任何玩家");
        return;
    }

    auto pCon = this->CS2->client
        .GetBaseEntityFromHandle(MulNX::MRead(pOBing->m_hController()))
        ->As<CS2::CCSPlayerController>();
    if (!pCon)return;

    auto playerName = MulNX::Memory::ReadString(pCon->m_iszPlayerName()).value_or("读取失败");
    auto uid = MulNX::MRead(pCon->m_steamID());

    ImGui::Text(std::format("当前正在观战玩家： {}", playerName).c_str());
    ImGui::Text(std::format("玩家64位Steam uid： {}", uid).c_str());

    MulNX::Message msg;
    auto&& [ruid] = msg.Access<Steam64UID>();
    ruid = uid;
    uico->CallbackCall("UI.Player.Info"_hash, &msg);
}

bool SpecTargetUI::Init() {
    this->SendUIRoot(this->GetName(), [this](auto uico, auto&&...) {
        try {
            this->Window(uico);
        }
        catch (MulNX::Exception& e) {
            this->LogWarning(e);
        }
        });

    return true;
}