#include "RecordTaskMaker.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/CS2/DemoSystem/RecordTaskConfiger/RecordTaskConfiger.hpp>
#include <MulNXExtensions/CS2/CSController/CSController.hpp>

bool RecordTaskMaker::Window(MulNX::UINode* node) {
    auto w = MulNX::UI::RAIIWindow("录制任务创建", this->showWindow);
    if (!w) return true;
    node->CallUINode("DemoJSONReader");
    MulNX::UI::SmartButton btn{};

    std::shared_lock lock(this->smutex);
    ImGui::Text("选择demo：");
    for (const auto& [name, demo] : this->demos) {
        if (ImGui::Button(name.c_str())) {
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/SetOperating"_hash);
            rp->str1 = name;
            this->ISys().PublishAsync(std::move(msg));
        }
    }

    auto it = this->demos.find(this->currentDemoName);
    if (it == this->demos.end()) {
        ImGui::Text("未选中任何demo");
        return true;
    }
    const auto& demoInfo = it->second;

    // 比赛基本信息表格，这里多显示比赛元数据
    if (ImGui::BeginTable("DemoInfo", 2, ImGuiTableFlags_Borders)) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0); ImGui::Text("Map");
        ImGui::TableSetColumnIndex(1); ImGui::Text("%s", demoInfo.mapName.c_str());

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0); ImGui::Text("Ticks");
        ImGui::TableSetColumnIndex(1); ImGui::Text("%d", demoInfo.tickCount);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0); ImGui::Text("Team A");
        ImGui::TableSetColumnIndex(1); ImGui::Text("%s (%s) - Score: %d",
            demoInfo.teamA.name.c_str(), demoInfo.teamA.letter.c_str(), demoInfo.teamA.score);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0); ImGui::Text("Team B");
        ImGui::TableSetColumnIndex(1); ImGui::Text("%s (%s) - Score: %d",
            demoInfo.teamB.name.c_str(), demoInfo.teamB.letter.c_str(), demoInfo.teamB.score);

        ImGui::EndTable();
    }

    ImGui::Separator();

    ImGui::Text("Players:");

    static Steam64UID ctrlTarget = 0;
    auto showPlayers = [&](bool wantA) {
        // 玩家统计表（Player/SteamID/Team/K/D/A）
        auto t = MulNX::UI::RAIITable("PlayersTable", { "Player" ,"SteamID" ,"K/D/A" },
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
        if (!t)return;

        for (const auto& [steamID, player] : demoInfo.players) {
            if (player.isTeamA != wantA)continue;
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("%s", player.name.c_str());
            ImGui::TableSetColumnIndex(1);
            if (ImGui::Button(std::to_string(player.steamId).c_str())) {
                ctrlTarget = player.steamId;
            }
            ImGui::TableSetColumnIndex(2); ImGui::Text(std::format("{}--{}--{}", player.killCount, player.deathCount, player.assistCount).c_str());
        }
        };

    ImGui::SeparatorText("A队");
    showPlayers(true);
    ImGui::SeparatorText("B队");
    showPlayers(false);


    ImGui::Separator();

    auto itPlayer = demoInfo.players.find(ctrlTarget);
    if (itPlayer == demoInfo.players.end()) {
        ImGui::SeparatorText("未选中玩家");
        return true;
    }
    const auto& player = itPlayer->second;
    ImGui::SeparatorText(player.name.c_str());
    ImGui::Checkbox("展示被击杀记录", &this->showBekillEvent);

    for (const auto& [round, info] : player.roundInfo) {
        if (!this->showBekillEvent && info.killEvents.empty())continue;

        ImGui::SeparatorText(std::format("第 {} 回合", round).c_str());

        // 统一的击杀事件表
        auto t = MulNX::UI::RAIITable("KillEventsTable", { "操作","Tick","被击杀者","助攻者","所用武器" },
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
        if (!t)continue;

        for (const auto& ev : info.killEvents) {
            ImGui::TableNextRow();

            // 第一列：录制按钮（需要唯一 ID）
            ImGui::TableSetColumnIndex(0);
            if (btn.Next("录制")) {
                auto [msg, rp] = MulNX::Message::Create<RecordTask>("Demo/Record/Enqueue"_hash);
                rp->uid = ev.killerSteamId;
                rp->tickStart = ev.tick - this->pConfiger->preRecordTicks;
                rp->tickEnd = ev.tick + this->pConfiger->postRecordTicks;
                auto desc = std::format("玩家 {} 击杀了 玩家 {}，录制从 {} tick到 {} tick",
                    demoInfo.GetPlayerName(ev.killerSteamId),
                    demoInfo.GetPlayerName(ev.victimSteamId),
                    rp->tickStart, rp->tickEnd);
                rp->desc = std::move(desc);

                float* pTimeScale = this->CS2()->GetCvarSystem().GetCvar("host_timescale")->GetPtr<float>();

                this->ISys().PublishAsync(std::move(msg));
            }
            ImGui::SameLine();
            if (btn.Next("受害者视角")) {
                auto [msg, rp] = MulNX::Message::Create<RecordTask>("Demo/Record/Enqueue"_hash);
                rp->uid = ev.victimSteamId;                // 录制视角 = 被击杀者
                rp->tickStart = ev.tick - this->pConfiger->preRecordTicksBekilled;
                rp->tickEnd = ev.tick + this->pConfiger->postRecordTicksBekilled;
                rp->desc = std::format("玩家 {} 被 玩家 {} 击杀，录制从 {} tick到 {} tick",
                    demoInfo.GetPlayerName(ev.victimSteamId),
                    demoInfo.GetPlayerName(ev.killerSteamId),
                    rp->tickStart, rp->tickEnd);
                this->ISys().PublishAsync(std::move(msg));
            }

            // 后续各列：击杀事件的具体字段
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%d", ev.tick);

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s", demoInfo.GetPlayerName(ev.victimSteamId).c_str());

            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%s", demoInfo.GetPlayerName(ev.assisterSteamId).c_str());

            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%s", ev.weaponName.c_str());
        }

        if (this->showBekillEvent && info.Bekilled.has_value()) {
            ImGui::TableNextRow();
            const auto& ev = info.Bekilled.value();

            // 列0：录制按钮（这次录制玩家自己被击杀的片段）
            ImGui::TableSetColumnIndex(0);
            if (btn.Next("录制被击杀")) {
                auto [msg, rp] = MulNX::Message::Create<RecordTask>("Demo/Record/Enqueue"_hash);
                rp->uid = ev.victimSteamId;                // 录制视角 = 被击杀者（玩家自己）
                rp->tickStart = ev.tick - this->pConfiger->preRecordTicksBekilled;
                rp->tickEnd = ev.tick + this->pConfiger->postRecordTicksBekilled;
                rp->desc = std::format("玩家 {} 被 玩家 {} 击杀，录制从 {} tick到 {} tick",
                    demoInfo.GetPlayerName(ev.victimSteamId),
                    demoInfo.GetPlayerName(ev.killerSteamId),
                    rp->tickStart, rp->tickEnd);
                this->ISys().PublishAsync(std::move(msg));
            }

            ImGui::TableSetColumnIndex(1); ImGui::Text("%d", ev.tick);
            ImGui::TableSetColumnIndex(2); ImGui::Text(std::format("{} (由 {} 击杀)", demoInfo.GetPlayerName(ev.victimSteamId), demoInfo.GetPlayerName(ev.killerSteamId)).c_str());
            ImGui::TableSetColumnIndex(3); ImGui::Text("%s", demoInfo.GetPlayerName(ev.assisterSteamId).c_str());
            ImGui::TableSetColumnIndex(4); ImGui::Text("%s", ev.weaponName.c_str());
        }
    }
    return true;
}

bool RecordTaskMaker::Init() {
    this->pConfiger = this->Core->ModuleManager()->FindModule<RecordTaskConfiger>("RecordTaskConfiger");

    this->ISys()
        .SubscribeAsync("Demo/SetOperating")
        .SubscribeAsync("Demo/InfoLoad");

    this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {return this->Window(node);});

    this->ISys().SendTask("Update", "DemoSys", [this]()->bool {
        this->Update();
        return true;
        });

    this->showWindow.store(true, std::memory_order_release);
    return true;
}

void RecordTaskMaker::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Demo/InfoLoad"_hash: {
        std::unique_lock lock(this->smutex);
        auto demoInfo = std::move(*msg.asp.get<Demo::Info>());
        this->demos[demoInfo.demoFileName] = std::move(demoInfo);
        break;
    }
    case "Demo/SetOperating"_hash: {
        std::unique_lock lock(this->smutex);
        this->currentDemoName = msg.asp.get<MulNX::NetExt>()->str1;
        break;
    }
    }
}