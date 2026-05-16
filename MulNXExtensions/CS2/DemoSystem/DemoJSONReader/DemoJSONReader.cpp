#include "DemoJSONReader.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/CS2/DemoSystem/DemoRecorder/DemoRecorder.hpp>
#include <fstream>
#include <nlohmann/json.hpp>

class NomalRecordTask :public IRecordTask {
public:
    std::string desc;
    Steam64UID uid;
    int tick;

    ~NomalRecordTask() = default;

    Steam64UID GetTargetSteam64UID()override { return this->uid; }
    int GetTargetTick()override { return this->tick; }
    std::string& GetDesc()override { return this->desc; }
};

bool DemoJSONReader::Window(MulNX::UINode* node) {
    auto w = MulNX::UI::RAIIWindow("Demo JSON Reader", this->showWindow);
    if (!w) return true;

    std::shared_lock lock(this->smutex, std::defer_lock);
    if (this->smutex.try_lock_shared()) {
        lock = std::shared_lock(this->smutex, std::adopt_lock);
    }
    else {
        ImGui::Text("正在读取...");
        return true;
    }

    // 1. 比赛基本信息表格（保持原样）
    if (ImGui::BeginTable("DemoInfo", 2, ImGuiTableFlags_Borders)) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0); ImGui::Text("Map");
        ImGui::TableSetColumnIndex(1); ImGui::Text("%s", this->demoInfo.mapName.c_str());

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0); ImGui::Text("Ticks");
        ImGui::TableSetColumnIndex(1); ImGui::Text("%d", this->demoInfo.tickCount);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0); ImGui::Text("Team A");
        ImGui::TableSetColumnIndex(1); ImGui::Text("%s (%s) - Score: %d",
            this->demoInfo.teamA.name.c_str(), this->demoInfo.teamA.letter.c_str(), this->demoInfo.teamA.score);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0); ImGui::Text("Team B");
        ImGui::TableSetColumnIndex(1); ImGui::Text("%s (%s) - Score: %d",
            this->demoInfo.teamB.name.c_str(), this->demoInfo.teamB.letter.c_str(), this->demoInfo.teamB.score);

        ImGui::EndTable();
    }

    ImGui::Separator();
    ImGui::Text("Players:");

    // 2. 玩家统计表（保持原样：Player/SteamID/Team/K/D/A）
    if (ImGui::BeginTable("PlayersTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Player");
        ImGui::TableSetupColumn("SteamID");
        ImGui::TableSetupColumn("Team");
        ImGui::TableSetupColumn("K");
        ImGui::TableSetupColumn("D");
        ImGui::TableSetupColumn("A");
        ImGui::TableHeadersRow();

        for (const auto& [steamID, player] : this->demoInfo.players) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("%s", player.name.c_str());
            ImGui::TableSetColumnIndex(1); ImGui::Text("%llu", player.steamId);
            ImGui::TableSetColumnIndex(2); ImGui::Text("%s", player.isTeamA ? "A" : "B");
            ImGui::TableSetColumnIndex(3); ImGui::Text("%d", player.killCount);
            ImGui::TableSetColumnIndex(4); ImGui::Text("%d", player.deathCount);
            ImGui::TableSetColumnIndex(5); ImGui::Text("%d", player.assistCount);
        }
        ImGui::EndTable();
    }

    ImGui::Separator();
    ImGui::Text("Kill Events:");

    // 3. 统一的击杀事件表（按玩家分组，第一行显示 SteamID，后续行第一列留空）
    if (ImGui::BeginTable("KillEventsTable", 6,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
        // 列定义：
        // [0] Player (SteamID)   [1] Tick   [2] Killer   [3] Victim   [4] Assister   [5] Weapon
        ImGui::TableSetupColumn("Player");
        ImGui::TableSetupColumn("Tick");
        ImGui::TableSetupColumn("Killer");
        ImGui::TableSetupColumn("Victim");
        ImGui::TableSetupColumn("Assister");
        ImGui::TableSetupColumn("Weapon");
        ImGui::TableHeadersRow();

        for (const auto& [steamID, player] : this->demoInfo.players) {
            const auto& events = player.killEvents;

            // 玩家独占一行（作为组头）
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", this->demoInfo.GetPlayerName(steamID).c_str());
            // 其余列保持为空以突出玩家行
            for (int c = 1; c < 6; ++c) { ImGui::TableSetColumnIndex(c); ImGui::TextUnformatted(""); }

            if (events.empty()) {
                continue;
            }

            // 每个击杀事件占独立行，第一列为录制按钮
            for (const auto& ev : events) {
                ImGui::TableNextRow();

                // 第一列：录制按钮（需要唯一 ID）
                ImGui::TableSetColumnIndex(0);
                std::string btnLabel = std::string("录制##") + std::to_string(ev.victimSteamId) + "_" + std::to_string(ev.tick);
                if (ImGui::Button(btnLabel.c_str())) {
                    auto [msg, rp] = MulNX::Message::Create<std::unique_ptr<NomalRecordTask>>("Demo/Record/Enqueue"_hash);
                    auto desc = this->demoInfo.GetPlayerName(ev.killerSteamId) + "  killed  " + this->demoInfo.GetPlayerName(ev.victimSteamId);
                    auto& task = *rp;
                    task = std::make_unique<NomalRecordTask>();
                    task->desc = std::move(desc);
                    task->uid = ev.killerSteamId;
                    task->tick = ev.tick;
                    this->ISys().PublishAsync(std::move(msg));
                }

                // 后续各列：击杀事件的具体字段
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%d", ev.tick);

                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%s", this->demoInfo.GetPlayerName(ev.killerSteamId).c_str());

                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%s", this->demoInfo.GetPlayerName(ev.victimSteamId).c_str());

                ImGui::TableSetColumnIndex(4);
                ImGui::Text("%s", this->demoInfo.GetPlayerName(ev.assisterSteamId).c_str());

                ImGui::TableSetColumnIndex(5);
                ImGui::Text("%s", ev.weaponName.c_str());
            }
        }
        ImGui::EndTable();
    }

    return true;
}

bool DemoJSONReader::Init() {
    this->dirDemos = this->ISys().PathManager()->PathGetForShared("Demos");
    this->ISys().SubscribeAsync("Demo/JSON/Load");

    this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {
        return this->Window(node);
        });

    this->SendTask("DemoSys", [this]()->bool {
        this->Update();
        return true;
        });

    this->showWindow.store(true, std::memory_order_release);

    return true;
}

void DemoJSONReader::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Demo/JSON/Load"_hash: {
        auto* pNetExt = msg.asp.get<MulNX::NetExt>();
        std::string filename = pNetExt->str1 + ".json";

        std::unique_lock lock(this->smutex);
        std::filesystem::path filePath = this->dirDemos / filename;
        if (!std::filesystem::exists(filePath)) {
            this->ISys().LogError("文件不存在: " + filePath.string());
            break;
        }
        try {
            auto json = nlohmann::json::parse(std::ifstream(filePath));
            this->ISys().LogInfo("成功读取 JSON 文件: " + filePath.string());
            Demo::Info info{};
            info.mapName = json["mapName"].get<std::string>();
            info.tickCount = json["tickCount"].get<int>();

            info.teamA.name = json["teamA"]["name"].get<std::string>();
            info.teamA.letter = json["teamA"]["letter"].get<std::string>();
            info.teamA.score = json["teamA"]["score"].get<int>();

            info.teamB.name = json["teamB"]["name"].get<std::string>();
            info.teamB.letter = json["teamB"]["letter"].get<std::string>();
            info.teamB.score = json["teamB"]["score"].get<int>();

            auto playersJson = json["players"];
            for (auto it = playersJson.begin(); it != playersJson.end(); ++it) {
                const auto& playerJson = it.value();
                Demo::Player player;
                player.steamId = playerJson["steamId"].get<Steam64UID>();
                if (player.steamId == 0)continue;
                player.name = playerJson["name"].get<std::string>();
                auto team = playerJson["team"];
                auto letter = team["letter"].get<std::string>();
                player.isTeamA = (letter == info.teamA.letter);
                player.killCount = playerJson["killCount"].get<int>();
                player.deathCount = playerJson["deathCount"].get<int>();
                player.assistCount = playerJson["assistCount"].get<int>();

                info.players[player.steamId] = std::move(player);
            }

            auto killsJson = json["kills"];
            for (auto it = killsJson.begin(); it != killsJson.end(); ++it) {
                const auto& killJson = it.value();
                Demo::KillEvent killEvent{};
                killEvent.tick = killJson["tick"].get<int>();
                killEvent.killerSteamId = killJson["killerSteamId"].get<Steam64UID>();
                killEvent.assisterSteamId = killJson["assisterSteamId"].get<Steam64UID>();
                killEvent.victimSteamId = killJson["victimSteamId"].get<Steam64UID>();

                if (killEvent.killerSteamId == 0 || killEvent.victimSteamId == 0) {
                    continue;
                }
                if (killEvent.killerSteamId == killEvent.victimSteamId) {
                    continue;
                }

                killEvent.weaponName = killJson["weaponName"].get<std::string>();
                info.players[killEvent.killerSteamId].killEvents.push_back(std::move(killEvent));
            }
            this->demoInfo = std::move(info);
        }
        catch (const std::exception& e) {
            this->ISys().LogError("读取 JSON 文件时发生错误: " + filePath.string());
        }
        break;
    }
    }
}