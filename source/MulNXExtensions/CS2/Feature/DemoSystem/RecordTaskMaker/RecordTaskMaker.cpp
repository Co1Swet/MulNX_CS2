#include "RecordTaskMaker.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <Intro/HookConsole/HookConsole.hpp>
#include <Feature/DemoSystem/RecordTaskConfiger/RecordTaskConfiger.hpp>

bool RecordTaskMaker::Window(MulNX::UICoordinator* uico) {
    auto w = MulNX::UI::RAIIWindow("录制任务创建");
    uico->CallUINode("DemoJSONReader");
    MulNX::UI::SmartButton btn{};

    std::shared_lock lock(this->smutex);
    ImGui::Text("选择demo：");
    for (const auto& [name, demo] : this->demos) {
        if (ImGui::Button(name.c_str())) {
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/SetOperating"_hash);
            rp->str1 = name;
            this->PublishAsync(std::move(msg));
        }
    }

    auto it = this->demos.find(this->currentDemoName);
    if (it == this->demos.end()) {
        ImGui::Text("未选中任何demo");
        return true;
    }
    const auto& demoInfo = it->second;

    // 比赛基本信息表格
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

        // ---------- 邻近合并预处理 ----------
        struct MergeInfo {
            int startTick;   // 合并后录制起始 tick（最早事件 tick - preRecordTicks）
            int endTick;     // 合并后录制结束 tick（最晚事件 tick + postRecordTicks）
            size_t eventCount;
        };
        const auto& killEvents = info.killEvents;
        std::vector<std::optional<MergeInfo>> mergeAfter(killEvents.size());

        if (!killEvents.empty()) {
            size_t start = 0;
            for (size_t i = 1; i <= killEvents.size(); ++i) {
                // i == killEvents.size() 时强制结束上一组
                bool endGroup = (i == killEvents.size()) ||
                    (killEvents[i].tick - killEvents[i - 1].tick > this->pConfiger->mergeThresholdTicks);

                if (endGroup) {
                    size_t end = i - 1;
                    if (end > start) { // 至少两个事件才合并
                        int startTick = killEvents[start].tick - this->pConfiger->preRecordTicks;
                        int endTick = killEvents[end].tick + this->pConfiger->postRecordTicks;
                        mergeAfter[end] = MergeInfo{ startTick, endTick, end - start + 1 };
                    }
                    start = i;
                }
            }
        }

        // ---------- 渲染击杀事件表 ----------
        auto t = MulNX::UI::RAIITable("KillEventsTable", { "操作","Tick","被击杀者","助攻者","所用武器" },
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
        if (!t)continue;

        for (size_t idx = 0; idx < killEvents.size(); ++idx) {
            const auto& ev = killEvents[idx];
            ImGui::TableNextRow();

            // 列0：单事件录制按钮
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
                this->PublishAsync(std::move(msg));
            }
            ImGui::SameLine();
            if (btn.Next("受害者视角")) {
                auto [msg, rp] = MulNX::Message::Create<RecordTask>("Demo/Record/Enqueue"_hash);
                rp->uid = ev.victimSteamId;
                rp->tickStart = ev.tick - this->pConfiger->preRecordTicksBekilled;
                rp->tickEnd = ev.tick + this->pConfiger->postRecordTicksBekilled;
                rp->desc = std::format("玩家 {} 被 玩家 {} 击杀，录制从 {} tick到 {} tick",
                    demoInfo.GetPlayerName(ev.victimSteamId),
                    demoInfo.GetPlayerName(ev.killerSteamId),
                    rp->tickStart, rp->tickEnd);
                this->PublishAsync(std::move(msg));
            }

            // 列1-4：事件信息
            ImGui::TableSetColumnIndex(1); ImGui::Text("%d", ev.tick);
            ImGui::TableSetColumnIndex(2); ImGui::Text("%s", demoInfo.GetPlayerName(ev.victimSteamId).c_str());
            ImGui::TableSetColumnIndex(3); ImGui::Text("%s", demoInfo.GetPlayerName(ev.assisterSteamId).c_str());
            ImGui::TableSetColumnIndex(4); ImGui::Text("%s", ev.weaponName.c_str());

            // ---------- 插入合并按钮（若当前事件是某合并组的最后一个）----------
            if (mergeAfter[idx].has_value()) {
                const auto& mi = mergeAfter[idx].value();
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                // 合并按钮：录制选中玩家的视角
                if (btn.Next("合并记录")) {
                    auto [msg, rp] = MulNX::Message::Create<RecordTask>("Demo/Record/Enqueue"_hash);
                    rp->uid = ctrlTarget;   // 当前查看的玩家
                    rp->tickStart = mi.startTick;
                    rp->tickEnd = mi.endTick;
                    rp->desc = std::format("玩家 {} 第 {} 回合合并录制 {} 个击杀事件，从 {} tick 到 {} tick",
                        demoInfo.GetPlayerName(ctrlTarget), round,
                        mi.eventCount, rp->tickStart, rp->tickEnd);
                    this->PublishAsync(std::move(msg));
                }
                // 其余列可空或显示信息
                ImGui::TableSetColumnIndex(1); ImGui::Text("(%d 事件)", mi.eventCount);
            }
        }

        // ---------- 被击杀事件（不参与合并）----------
        if (this->showBekillEvent && info.Bekilled.has_value()) {
            const auto& ev = info.Bekilled.value();
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            if (btn.Next("录制被击杀")) {
                auto [msg, rp] = MulNX::Message::Create<RecordTask>("Demo/Record/Enqueue"_hash);
                rp->uid = ev.victimSteamId;
                rp->tickStart = ev.tick - this->pConfiger->preRecordTicksBekilled;
                rp->tickEnd = ev.tick + this->pConfiger->postRecordTicksBekilled;
                rp->desc = std::format("玩家 {} 被 玩家 {} 击杀，录制从 {} tick到 {} tick",
                    demoInfo.GetPlayerName(ev.victimSteamId),
                    demoInfo.GetPlayerName(ev.killerSteamId),
                    rp->tickStart, rp->tickEnd);
                this->PublishAsync(std::move(msg));
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

    (*this)
        .SubscribeAsync("Demo/SetOperating")
        .SubscribeAsync("Demo/InfoLoad");

    this->UIRegisterCallback("UI.Demos", [this](auto uico, auto&&...) {return this->Window(uico);});

    this->SendTask("Update", "DemoSys", [this]()->bool {
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
        auto demoInfo = *msg.asp.get<Demo::Info>();
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