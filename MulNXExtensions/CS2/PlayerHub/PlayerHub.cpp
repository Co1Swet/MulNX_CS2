#include "PlayerHub.hpp"

#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/CS2/CSController/CSController.hpp>

bool PlayerHub::Window(MulNX::UINode* node) {
    auto w = MulNX::UI::RAIIWindow("玩家信息管理", this->ShowWindow);
    if (!w) return false;
    try {
        std::shared_lock lock(this->smutex);

        // ---------- 玩家列表区域 ----------
        ImGui::TextUnformatted("检测到如下玩家信息：");
        static int showMax = 20;
        ImGui::SliderInt("搜索的最大数量", &showMax, 1, 255);

        int playerNum = 0;

        // 用于按队伍分组存储的临时结构
        struct PlayerInfo {
            uint64_t steamID;
            std::string displayName;
            CS2::CCSPlayerController* controller;
            CS2::ui8TeamNum teamNum;
        };
        std::vector<PlayerInfo> ctPlayers;
        std::vector<PlayerInfo> tPlayers;

        for (int i = 0; i <= std::min(this->CS2()->client.dwGameEntitySystem_highestEntityIndex(), showMax); ++i) {
            auto* baseEntity = this->CS2()->client.GetBaseEntity(i);
            if (!baseEntity) continue;

            auto* playerController = baseEntity->As<CS2::CCSPlayerController>();
            if (!playerController) continue;

            auto hPawn = MulNX::MRead(playerController->m_hPlayerPawn());
            auto* pawn = this->CS2()->client.GetBaseEntityFromHandle(hPawn)->As<CS2::C_CSPlayerPawn>();
            if (!pawn) continue;

            uint64_t SteamID = MulNX::MRead(playerController->m_steamID());
            if (SteamID == 0) continue;

            ++playerNum;
            std::string displayName = std::format("玩家 {} (SteamID: {})", playerNum, SteamID);
            auto teamNum = MulNX::MRead(playerController->iTeamNum());

            // 根据队伍存储
            if (teamNum == CS2::ui8TeamNum::CT) {
                ctPlayers.push_back({ SteamID, displayName, playerController, teamNum });
            }
            else if (teamNum == CS2::ui8TeamNum::T) {
                tPlayers.push_back({ SteamID, displayName, playerController, teamNum });
            }
            // 注意：其他队伍（如观察者）不会被显示，但 playerNum 依然会递增，符合要求
        }

        // 辅助 lambda 用于绘制玩家条目（保持原有交互逻辑）
        auto DrawPlayerEntry = [&](const PlayerInfo& info) {
            if (ImGui::Selectable(info.displayName.c_str(),
                this->currentSteamId.load(std::memory_order_acquire) == info.steamID)
                ) {
                this->currentSteamId.store(info.steamID, std::memory_order_release);
                this->ShowCompanionWindow.store(true, std::memory_order_release);
                this->currentTeam.store(info.teamNum, std::memory_order_release);
            }
            auto naturalName = MulNX::Memory::ReadString(info.controller->m_iszPlayerName());
            ImGui::TextUnformatted(std::format("自然名字: {}", naturalName).c_str());
            ImGui::Separator();
            };

        // ---------- 左右并排绘制 CT 和 T 区域 ----------
        // 计算左右子区域宽度（各占一半，留出间距）
        float childWidth = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

        // 左子区域：CT
        ImGui::BeginChild("CT_List", ImVec2(childWidth, 0));
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "反恐精英 (CT)");
        ImGui::Separator();
        if (!ctPlayers.empty()) {
            for (const auto& info : ctPlayers) {
                DrawPlayerEntry(info);
            }
        }
        else {
            ImGui::TextDisabled("无 CT 玩家");
        }
        ImGui::EndChild();

        ImGui::SameLine();

        // 右子区域：T
        ImGui::BeginChild("T_List", ImVec2(childWidth, 0));
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "恐怖分子 (T)");
        ImGui::Separator();
        if (!tPlayers.empty()) {
            for (const auto& info : tPlayers) {
                DrawPlayerEntry(info);
            }
        }
        else {
            ImGui::TextDisabled("无 T 玩家");
        }
        ImGui::EndChild();

        // 附加子窗口
        auto pos = ImGui::GetWindowPos();
        auto size = ImGui::GetWindowSize();
        ImGui::SetNextWindowPos(ImVec2(pos.x + size.x, pos.y));

        MulNX::UI::RAIIWindow infoWindow("玩家信息", this->ShowCompanionWindow);
        if (!infoWindow) return true;
        if (ImGui::BeginTabBar("视图选项")) {
            if (ImGui::BeginTabItem("玩家视图（高优先级）")) {
                this->showView.store(PlayerHub::View::Player, std::memory_order_release);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("队伍视图（低优先级）")) {
                this->showView.store(PlayerHub::View::Team, std::memory_order_release);
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::SeparatorText("修改");
        if (this->showView.load(std::memory_order_acquire) == PlayerHub::View::Player) {

            ImGui::TextUnformatted(std::format("当前选中的 SteamID: {}", this->currentSteamId.load(std::memory_order_acquire)).c_str());
            if (this->currentSteamId.load(std::memory_order_acquire) == 0) {
                ImGui::TextDisabled("请先在列表中选择一名玩家");
                return true;
            }
        }
        else {
            ImGui::TextUnformatted(std::format("当前选中的 队伍: {}",
                (this->currentTeam.load(std::memory_order_acquire) == CS2::ui8TeamNum::CT) ? "CT" : "T"
            ).c_str());
            if (this->currentSteamId.load(std::memory_order_acquire) == 0) {
                ImGui::TextDisabled("请先在列表中选择一名玩家");
                return true;
            }
        }

        ImGui::Separator();
        node->CallUINode("NameController");
        node->CallUINode("GlowController");
        node->CallUINode("SmokeController");
    }
    catch (const std::exception& e) {
        this->ISys().LogWarning(std::format("在绘制玩家信息时捕获到异常：{}", e.what()));
    }
    return true;
}

bool PlayerHub::Init() {
    this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {return this->Window(node);});
    return true;
}