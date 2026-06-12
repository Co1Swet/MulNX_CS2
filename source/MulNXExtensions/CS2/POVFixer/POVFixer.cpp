#include "POVFixer.hpp"
#include <MulNX/Base/UI/UI.hpp>

void POVFixer::Draw(MulNX::UINode* node) {
    if (ImGui::CollapsingHeader("POV Fix")) {
        if (ImGui::Button(I18n("pov.enable").c_str())) {
            this->ISys().PublishAsync("POVFix/Enable"_hash);
        }
        ImGui::SameLine();
        if (ImGui::Button(I18n("pov.disable").c_str())) {
            this->ISys().PublishAsync("POVFix/Disable"_hash);
        }
        MulNX::UI::Checkbox("Team ID隐藏敌方", this->pTeamIDController->runFlag1);
    }
}

bool POVFixer::Init() {
    this->pTeamIDController = this->Core->ModuleManager()->FindModule("TeamIDController");

    this->ISys()
        .SubscribeSync("Call/BeforeDraw", [this](MulNX::Message& msg) {this->BeforeDraw();})
        .SubscribeSync("Call/OnSetGlow", [this](MulNX::Message& msg) {this->OnSetGlow(msg);})
        .SubscribeAsync("POVFix/Enable")
        .SubscribeAsync("POVFix/Disable")
        ;

    this->ISys().SendUINode(this->GetName(), [this](MulNX::UINode* node) {return this->Draw(node);});
    
    return true;
}

void POVFixer::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "POVFix/Enable"_hash: {
        this->runFlag1.store(true);
        this->ISys().AsyncCommand("cl_radar_show_all_players_when_spectating false");
        this->ISys().AsyncCommand("cl_radar_square_always false");
        this->ISys().AsyncCommand("cl_radar_square_when_spectating false");
        this->ISys().AsyncCommand("cl_demo_predict 0");
        this->ISys().AsyncCommand("cl_spec_show_bindings false");
        break;
    }
    case "POVFix/Disable"_hash: {
        this->runFlag1.store(false);
        this->ISys().AsyncCommand("cl_radar_show_all_players_when_spectating true");
        this->ISys().AsyncCommand("cl_radar_square_when_spectating true");
        break;
    }
    }
}

void POVFixer::BeforeDraw() {
    this->Update();
}

void POVFixer::OnSetGlow(MulNX::Message& msg) {
    if (!this->runFlag1.load())return;
    auto team = msg.p1.low<CS2::ui8TeamNum>();
    *msg.p2.as<bool*>() = true;

    // try {
    //     auto pObservingPawn = this->CS2->client.TryGetObservingPawn();
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