#include "AutoAim.hpp"
#include <Buildup/TargetPicker/TargetPicker.hpp>

void AutoAim::Menu() {
    if (ImGui::Button("启动自瞄")) {
        this->AsyncCommand("MulNX/AutoAim/Enable");
    }
    ImGui::SameLine();
    if (ImGui::Button("关闭自瞄")) {
        this->AsyncCommand("MulNX/AutoAim/Disable");
    }
}

bool AutoAim::Init() {
    this->pTargetPicker = this->FindModule<TargetPicker>("TargetPicker");

    this->UIRegisterCallback("UI.CameraSetting", [this](auto&&...) {this->Menu();});

    (*this)
        .SubscribeAsync<void>("AutoAim/Enable")
        .SubscribeAsync<void>("AutoAim/Disable")
        ;

    return true;
}

void AutoAim::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "AutoAim/Enable"_hash: {
        if (this->pTargetPicker->UpdateTarget()) {
            this->enable = true;
        }
        break;
    }
    case "AutoAim/Disable"_hash: {
        this->enable = false;
        break;
    }
    }
}

bool AutoAim::HandleUpdateCSView(CS2::CViewSetup* viewSetup, const int& num, bool& camLeavePlayer) {
    this->Update();
    if (!this->enable.load(std::memory_order_acquire)) return false;
    try {
        auto steam64 = this->pTargetPicker->GetTarget();
        if (!steam64) return false;

        auto pTarCtrl = this->CS2Entitys->FindControllerBySteam64UID(steam64);
        if (!pTarCtrl) return false;

        auto hPawn = MulNX::MRead(pTarCtrl->m_hPlayerPawn());
        if (!hPawn.Valid()) return false;
        auto pPawn = this->CS2Entitys->GetBaseEntityFromHandle(hPawn)->As<CS2::C_CSPlayerPawn>();
        if (!pPawn) return false;
        if (MulNX::MRead(pPawn->iHealth()) == 0) return false;

        DirectX::XMFLOAT3 currentCamPos = *viewSetup->pViewOrigin();
        auto pAngle = viewSetup->pViewAngles();
        auto localAngle = this->CS2->client.dwViewAngles();
        if (!pAngle || !localAngle) return false;

        auto eyePos = pPawn->GetEyePos();

        // 计算指向目标的方向
        DirectX::XMFLOAT3 dirToTarget = {
            eyePos.x - currentCamPos.x,
            eyePos.y - currentCamPos.y,
            eyePos.z - currentCamPos.z
        };

        DirectX::XMFLOAT3 newAngles;
        MulNX::Math::CSDirToEuler(dirToTarget, newAngles);

        // 同时修改两个视角角度
        pAngle->x = newAngles.x;
        pAngle->y = newAngles.y;
        localAngle[0] = newAngles.x;
        localAngle[1] = newAngles.y;

        return true;
    }
    catch (const MulNX::Exception& e) {
        this->LogError(e);
    }
    return false;
}