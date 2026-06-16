#include "PlayerHub.hpp"
#include <MulNX/Base/UI/UI.hpp>

#include <MulNXExtensions/CS2/PlayerHub/CSViewPlayerModuleBase.hpp>

bool PlayerHub::Window(MulNX::UINode* node) {
    auto w = MulNX::UI::RAIIWindow("玩家信息管理", this->showWindow);
    if (!w) return false;
    try {
        std::shared_lock lock(this->smutex);

        // ---------- 玩家列表区域 ----------
        ImGui::TextUnformatted("检测到如下玩家信息：");
        static int showMax = 20;
        ImGui::SliderInt("搜索的最大数量", &showMax, 1, 255);

        int playerNum = 0;

        struct PlayerInfo {
            uint64_t steamID;
            std::string displayName;
            CS2::CCSPlayerController* controller;
            CS2::ui8TeamNum teamNum;
        };
        std::vector<PlayerInfo> ctPlayers;
        std::vector<PlayerInfo> tPlayers;

        for (int i = 0; i <= std::min(this->CS2->client.dwGameEntitySystem_highestEntityIndex(), showMax); ++i) {
            auto* baseEntity = this->CS2->client.GetBaseEntity(i);
            if (!baseEntity) continue;

            auto* playerController = baseEntity->As<CS2::CCSPlayerController>();
            if (!playerController) continue;

            uint64_t SteamID = MulNX::MRead(playerController->m_steamID());
            if (SteamID == 0) continue;

            ++playerNum;
            std::string displayName = std::format("玩家 {} (SteamID: {})", playerNum, SteamID);
            auto teamNum = MulNX::MRead(playerController->iTeamNum());

            if (teamNum == CS2::ui8TeamNum::CT) {
                ctPlayers.push_back({ SteamID, displayName, playerController, teamNum });
            }
            else if (teamNum == CS2::ui8TeamNum::T) {
                tPlayers.push_back({ SteamID, displayName, playerController, teamNum });
            }
        }

        // ---------- 绘制玩家条目 ----------
        auto DrawPlayerEntry = [&](const PlayerInfo& info) {
            if (ImGui::Selectable(info.displayName.c_str())) {}
            // 左键点击时触发玩家弹出菜单
            ImGui::OpenPopupOnItemClick(std::format("PlayerPopup_{:X}", info.steamID).c_str(),
                ImGuiPopupFlags_MouseButtonLeft);

            auto naturalName = MulNX::Memory::ReadString(info.controller->m_iszPlayerName());
            ImGui::TextUnformatted(std::format("自然名字: {}", naturalName).c_str());
            ImGui::Separator();
            };

        // ---------- 弹出菜单渲染 lambda ----------
        auto RenderPlayerPopup = [&](const PlayerInfo& info) {
            std::string popupName = std::format("PlayerPopup_{:X}", info.steamID);
            if (ImGui::BeginPopup(popupName.c_str())) {
                this->currentSteamId.store(info.steamID, std::memory_order_release);
                this->currentTeam.store(info.teamNum, std::memory_order_release);
                for (auto& mod : this->PlayerViewModules) {
                    mod->HubPlayer(node);
                }
                ImGui::Separator();
                if (ImGui::Button("关闭"))
                    ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }
            };

        auto RenderTeamPopup = [&](const char* name, CS2::ui8TeamNum team) {
            if (ImGui::BeginPopup(name)) {
                this->currentSteamId.store(0, std::memory_order_release);
                this->currentTeam.store(team, std::memory_order_release);
                for (auto& mod : this->PlayerViewModules) {
                    mod->HubTeam(node);
                }
                ImGui::Separator();
                if (ImGui::Button("关闭"))
                    ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }
            };

        float childWidth = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

        // 左：CT
        ImGui::BeginChild("CT_List", ImVec2(childWidth, 0));
        // 队伍名点击弹出队伍调节菜单
        if (ImGui::Selectable("反恐精英 (CT)")) {}
        ImGui::OpenPopupOnItemClick("TeamPopup_CT", ImGuiPopupFlags_MouseButtonLeft);
        ImGui::Separator();
        if (!ctPlayers.empty()) {
            for (const auto& info : ctPlayers) {
                DrawPlayerEntry(info);
            }
        }
        else {
            ImGui::TextDisabled("无 CT 玩家");
        }
        // --- 在 CT 子窗口内渲染所有 CT 相关弹出菜单 ---
        for (const auto& info : ctPlayers) {
            RenderPlayerPopup(info);
        }
        RenderTeamPopup("TeamPopup_CT", CS2::ui8TeamNum::CT);
        ImGui::EndChild();

        ImGui::SameLine();

        // 右：T
        ImGui::BeginChild("T_List", ImVec2(childWidth, 0));
        if (ImGui::Selectable("恐怖分子 (T)")) {}
        ImGui::OpenPopupOnItemClick("TeamPopup_T", ImGuiPopupFlags_MouseButtonLeft);
        ImGui::Separator();
        if (!tPlayers.empty()) {
            for (const auto& info : tPlayers) {
                DrawPlayerEntry(info);
            }
        }
        else {
            ImGui::TextDisabled("无 T 玩家");
        }
        // --- 在 T 子窗口内渲染所有 T 相关弹出菜单 ---
        for (const auto& info : tPlayers) {
            RenderPlayerPopup(info);
        }
        RenderTeamPopup("TeamPopup_T", CS2::ui8TeamNum::T);
        ImGui::EndChild();

        for (auto& mod : this->PlayerViewModules) {
            mod->HubWindow(node);
        }
    }
    catch (const std::exception& e) {
        this->LogWarning(std::format("在绘制玩家信息时捕获到异常：{}", e.what()));
    }
    return true;
}

bool PlayerHub::Init() {
    this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {return this->Window(node);});
    return true;
}