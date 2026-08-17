#include "EntityListScanner.hpp"
#include <MulNX/Base/UI/UI.hpp>


void EntityListScanner::Window() {
    auto w = MulNX::UI::RAIIWindow("实体列表探测器");
    if (!w || !w.ShouldDraw())return;

    for (int i = 0;i < 30;++i) {
        auto* entity = this->CS2->client.GetBaseEntity(i);
        if (!entity)continue;

        auto* controller = entity->As<CS2::CCSPlayerController>();

        auto hPawn = MulNX::MRead(controller->m_hPlayerPawn());

        auto* pawn = this->CS2->client.GetBaseEntityFromHandle(hPawn.GetIndexInEntityList())->As<CS2::C_CSPlayerPawn>();
        if (!pawn)continue;

        auto id = MulNX::MRead(controller->m_steamID());
        auto userName = MulNX::Memory::ReadString(controller->m_iszPlayerName()).value();

        auto info = std::format("检测到实体列表第{}项，是玩家{}，steam ID为{}", i, userName, id);
        ImGui::Text(info.c_str());
    }
}

bool EntityListScanner::Init() {
    this->SendUIRoot(this->GetName(), [this](auto&&...) {
        try {
            return this->Window();
        }
        catch (MulNX::Exception& e) {
            this->LogError(e);
        };
        });
    return true;
}