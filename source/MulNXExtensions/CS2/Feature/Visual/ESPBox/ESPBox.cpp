#include "ESPBox.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <Intro/HookView/HookView.hpp>

void ESPBox::Draw() {
    if (!this->enable.load(std::memory_order_acquire))return;

    for (int i = 0;i < 32;++i) {
        auto* pCtrl = this->CS2->client.GetBaseEntity(i)->As<CS2::CCSPlayerController>();
        if (!pCtrl)continue;
        auto hPawn = MulNX::MRead(pCtrl->m_hPlayerPawn());
        auto pPawn = this->CS2->client.GetBaseEntityFromHandle(hPawn)->As<CS2::C_CSPlayerPawn>();
        if (!pPawn)continue;
        if (!MulNX::MRead(pPawn->iHealth()))continue;

        auto pos3D = MulNX::MRead(pPawn->vOldOrigin());
        auto eyePos3D = pos3D + MulNX::MRead(pPawn->vecViewOffset());

        DirectX::XMFLOAT2 eyePos2D{};
        DirectX::XMFLOAT2 originPos2D{};

        if (!MulNX::Math::WorldToScreen(eyePos3D, eyePos2D, this->CS2View->GetViewMatrix(), this->CS2View->GetWinWidth(), this->CS2View->GetWinHeight()))
            continue;
        if (!MulNX::Math::WorldToScreen(pos3D, originPos2D, this->CS2View->GetViewMatrix(), this->CS2View->GetWinWidth(), this->CS2View->GetWinHeight()))
            continue;

        const float hight{ ::abs(eyePos2D.y - originPos2D.y) * 1.25f };
        const float width{ hight / 2.f };
        const float x = eyePos2D.x - (width / 2.f);
        const float y = eyePos2D.y - (width / 2.5f);

        ImGui::GetBackgroundDrawList()->AddRect(ImVec2(x, y), ImVec2(x + width, y + hight), ImColor(0, 255, 0, 255), 0.0f, 0, 1.5f);
    }
}

bool ESPBox::Init() {
    this->UIRegisterBackground(this->GetName(), [this](auto&&...) {
        try {
            this->Draw();
        }
        catch (MulNX::Exception& e) {
            this->LogError(e);
        };
        });

    this->UIRegisterCallback("UI.2DVision", [this](auto&&...) {
        MulNX::UI::Checkbox("方框ESP", this->enable);
        return true;
        });

    return true;
}