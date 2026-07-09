#include "EntityListScanner.hpp"
#include <MulNX/Base/UI/UI.hpp>


void EntityListScanner::Window(MulNX::UINode* node) {
    auto w = MulNX::UI::RAIIWindow("实体列表探测器");

    for (int i = 0;i < 30;++i) {
        try {
            auto* entity = this->CS2->client.GetBaseEntity(i);
            if (!entity)continue;

            auto* controller = entity->As<CS2::CCSPlayerController>();

            auto hPawn = MulNX::MRead(controller->m_hPlayerPawn());

            auto* pawn = this->CS2->client.GetBaseEntityFromHandle(hPawn.GetIndexInEntityList())->As<CS2::C_CSPlayerPawn>();
            if (!pawn)continue;

            auto id = MulNX::MRead(controller->m_steamID());
            auto userName = MulNX::Memory::ReadString(controller->m_iszPlayerName());

            auto info = std::format("检测到实体列表第{}项，是玩家{}，steam ID为{}", i, userName, id);
            ImGui::Text(info.c_str());

            // C_BaseEntity
            // ->Controller 控制器（玩家的现实里的身份，steamid）
            // ->Pawn 棋子（游戏里，在场上的人物）
        }
        catch (...) {
            continue;
        }
    }
}

bool EntityListScanner::Init() {
    this->SendUIRoot(this->GetName(), [this](MulNX::UINode* node) {
        return this->Window(node);
        });
    return true;
}