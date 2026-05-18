#include "POVFixer.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/CS2/CSController/CSController.hpp>

void POVFixer::Draw(MulNX::UINode* node) {
    if (ImGui::Button(I18n("pov.enable").c_str())) {
        this->ISys().PublishAsync("POVFix/Enable"_hash);
    }
    ImGui::SameLine();
    if (ImGui::Button(I18n("pov.disable").c_str())) {
        this->ISys().PublishAsync("POVFix/Disable"_hash);
    }
}

bool POVFixer::Init() {
    this->ISys()
        .SubscribeSync("Call/BeforeDraw", [this](MulNX::Message& msg) {this->BeforeDraw();})
        .SubscribeSync("Call/OnSetGlow", [this](MulNX::Message& msg) {this->OnSetGlow(msg);})
        .SubscribeAsync("POVFix/Enable")
        .SubscribeAsync("POVFix/Disable")
        ;

    this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {return this->Draw(node);});
    
    return true;
}

void POVFixer::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "POVFix/Enable"_hash: {
        this->enable = true;
        this->ISys().AsyncCommand("cl_radar_show_all_players_when_spectating false");
        this->ISys().AsyncCommand("cl_radar_square_always false");
        this->ISys().AsyncCommand("cl_radar_square_when_spectating false");
        this->ISys().AsyncCommand("cl_demo_predict 0");
        this->ISys().AsyncCommand("cl_spec_show_bindings false");
        break;
    }
    case "POVFix/Disable"_hash: {
        this->enable = false;
        this->ISys().AsyncCommand("cl_radar_show_all_players_when_spectating true");
        this->ISys().AsyncCommand("cl_radar_square_when_spectating true");
        break;
    }
    }
}

void POVFixer::BeforeDraw() {
    this->Update();
    if (!this->enable)return;

    try {
        auto pObservingPawn = this->CS2()->client.TryGetObservingPawn();
        if (!pObservingPawn)return;
        auto currentTeam = MulNX::MRead(pObservingPawn->iTeamNum());

        int playerNum = 0;
        for (int i = 0; i < this->CS2()->client.dwGameEntitySystem_highestEntityIndex(); ++i) {
            auto* controller = this->CS2()->client.GetBaseEntity(i)->As<CS2::CCSPlayerController>();
            if (!controller)continue;
            auto hPawn = MulNX::MRead(controller->m_hPlayerPawn());
            auto* pawn = this->CS2()->client.GetBaseEntityFromHandle(hPawn.GetIndexInEntityList())->As<CS2::C_CSPlayerPawn>();
            if (!pawn)continue;

            auto team = MulNX::MRead(pawn->iTeamNum());
            if (team != CS2::ui8TeamNum::T && team != CS2::ui8TeamNum::CT)continue;
            ++playerNum;

            if (playerNum == 20)return;

            if (currentTeam == team) {
                MulNX::MWrite(pawn->m_entitySpottedState()->m_bSpotted(), true);
            }
            else {
                MulNX::MWrite(pawn->m_entitySpottedState()->m_bSpotted(), false);
                auto pGlow = pawn->Glow()->bGlowing();
                MulNX::MWrite(pGlow, false);
            }
        }
        }
    catch (const std::exception& e) {
        this->ISys().LogError(e.what());
    }
}

void POVFixer::OnSetGlow(MulNX::Message& msg) {
    if (!this->enable)return;
    auto team = msg.p1.low<CS2::ui8TeamNum>();
    *msg.p2.as<bool*>() = true;

    // try {
    //     auto pObservingPawn = this->CS2()->client.TryGetObservingPawn();
    //     if (!pObservingPawn)return;
    //     auto currentTeam = MulNX::MRead(pObservingPawn->iTeamNum());
    //     if (team != currentTeam) {
            
    //     }
    // }
    // catch (const std::exception& e) {
    //     this->ISys().LogError(e.what());
    // }

    return;
}