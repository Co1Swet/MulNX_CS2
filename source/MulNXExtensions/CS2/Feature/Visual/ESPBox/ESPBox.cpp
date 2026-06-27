#include "ESPBox.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <Intro/HookView/HookView.hpp>

bool ESPBox::Draw(MulNX::UINode* node) {
    if (this->showWindow.load(std::memory_order_acquire)) {
        for (int i = 1; i <= 10; ++i) {
            if (!this->CS2->GetPlayerMsg(i).Alive)continue;
            const DirectX::XMFLOAT3 EyePos3D = this->CS2->GetPlayerMsg(i).EyePosition;
            const DirectX::XMFLOAT3 OriginPos3D = this->CS2->GetPlayerMsg(i).Position;

            DirectX::XMFLOAT2 EyePos2D{};
            DirectX::XMFLOAT2 OriginPos2D{};

            MulNX::Math::WorldToScreen(EyePos3D, EyePos2D, this->CS2View->GetViewMatrix(), this->CS2View->GetWinWidth(), this->CS2View->GetWinHeight());
            MulNX::Math::WorldToScreen(OriginPos3D, OriginPos2D, this->CS2View->GetViewMatrix(), this->CS2View->GetWinWidth(), this->CS2View->GetWinHeight());

            const float hight{ ::abs(EyePos2D.y - OriginPos2D.y) * 1.25f };
            const float width{ hight / 2.f };
            const float x = EyePos2D.x - (width / 2.f);
            const float y = EyePos2D.y - (width / 2.5f);

            ImGui::GetBackgroundDrawList()->AddRect(ImVec2(x, y), ImVec2(x + width, y + hight), ImColor(0, 255, 0, 255), 0.0f, 0, 1.5f);
        }
    }

    return true;
}

bool ESPBox::Init() {
    this->SendUIRoot(this->GetName(), [this](MulNX::UINode* node)->bool {
        return this->Draw(node);
        });

    return true;
}