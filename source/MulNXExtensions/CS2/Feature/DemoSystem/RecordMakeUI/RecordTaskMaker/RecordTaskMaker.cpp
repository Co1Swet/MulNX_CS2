#include "RecordTaskMaker.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <Intro/HookConsole/HookConsole.hpp>
#include <Feature/DemoSystem/RecordTaskConfiger/RecordTaskConfiger.hpp>

void RecordTaskMaker::Window(MulNX::UICoordinator* uico) {
    auto w = MulNX::UI::RAIIWindow("录制任务创建");
    if (!w || !w.ShouldDraw())return;
    std::shared_lock lock(this->smutex);

    auto pDemoInfo = this->DemoChooseMenu(uico);
    if (!pDemoInfo)return;
    ImGui::Separator();
    auto& demoInfo = *pDemoInfo;

    this->DemoMetaMenu(demoInfo);
    ImGui::Separator();

    auto ctrlTarget = this->TeamPlayerMenu(demoInfo);
    ImGui::Separator();

    auto itPlayer = demoInfo.players.find(ctrlTarget);
    if (itPlayer == demoInfo.players.end()) {
        ImGui::SeparatorText("未选中玩家");
        return;
    }

    const auto& player = itPlayer->second;
    ImGui::SeparatorText(player.name.c_str());
    ImGui::Checkbox("展示被击杀记录", &this->showBekillEvent);

    if (ImGui::Button(std::format("为所有回合生成录制任务##{}", player.steamId).c_str())) {
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/Record/Make/FullRoundAll"_hash);
        rp->str1 = pDemoInfo->demoFileName;
        auto&& [rUid] = msg.Access<Steam64UID>();
        rUid = player.steamId;
        this->PublishAsync(std::move(msg));
    }
    ImGui::SameLine();
    if (ImGui::Button(std::format("为所有击杀生成录制任务##{}", player.steamId).c_str())) {
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/Record/Make/RoundKillAll"_hash);
        rp->str1 = pDemoInfo->demoFileName;
        auto&& [rUid] = msg.Access<Steam64UID>();
        rUid = player.steamId;
        this->PublishAsync(std::move(msg));
    }

    for (int i = demoInfo.rounds.front().number;i <= demoInfo.rounds.back().number;++i) {
        ImGui::SeparatorText(std::format("第 {} 回合", i).c_str());

        if (ImGui::Button(std::format("录制整回合##round_{}", i).c_str())) {
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/Record/Make/FullRound"_hash);
            rp->str1 = pDemoInfo->demoFileName;
            auto&& [rUid, rRound] = msg.Access<Steam64UID, size_t>();
            rUid = player.steamId;
            rRound = i;
            this->PublishAsync(std::move(msg));
        }

        if (player.roundInfo.contains(i)) {
            this->RoundPlayerMenu(i, player, demoInfo);
        }
    }
}

std::shared_ptr<const Demo::Info> RecordTaskMaker::DemoChooseMenu(MulNX::UICoordinator* uico) {
    auto pPlaying = this->pDemSys->GetCurrentPlayingDemoName();
    if (pPlaying) {
        ImGui::SameLine();
        const auto& cp = *pPlaying;
        if (ImGui::Button(std::format("切换到当前播放Demo:{}", cp).c_str())) {
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/SetOperating"_hash);
            rp->str1 = cp;
            this->PublishAsync(std::move(msg));
        }
    }

    auto pOperating = this->pDemSys->GetCurrentOperatingDemoName();
    if (!pOperating) {
        ImGui::Text("未选中任何demo");
        return nullptr;
    }
    auto pDemo = this->pDemSys->GetDemoInfo(*pOperating);
    if (!pDemo) {
        ImGui::TextUnformatted(std::format("选中Demo：{} ，而尚未加载信息！", *pOperating).c_str());
        return nullptr;
    }
    return pDemo;
}

void RecordTaskMaker::DemoMetaMenu(const Demo::Info& demoInfo) {
    // 比赛基本信息表格
    if (ImGui::BeginTable("DemoInfo", 2)) {
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
}

Steam64UID RecordTaskMaker::TeamPlayerMenu(const Demo::Info& demoInfo) {
    ImGui::Text("Players:");

    static Steam64UID ctrlTarget = 0;
    auto showPlayers = [&](bool wantA) {
        auto t = MulNX::UI::RAIITable("PlayersTable", { "Player" ,"SteamID" ,"K/D/A" });
        if (!t)return;

        for (const auto& [steamID, player] : demoInfo.players) {
            if (player.isTeamA != wantA)continue;
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("%s", player.name.c_str());
            ImGui::TableSetColumnIndex(1);
            if (ImGui::Button(std::to_string(player.steamId).c_str())) {
                ctrlTarget = player.steamId;
            }
            ImGui::TableSetColumnIndex(2); ImGui::TextUnformatted(std::format("{}--{}--{}", player.killCount, player.deathCount, player.assistCount).c_str());
        }
        };

    ImGui::SeparatorText("A队");
    showPlayers(true);
    ImGui::SeparatorText("B队");
    showPlayers(false);

    return ctrlTarget;
}

void RecordTaskMaker::RoundPlayerMenu(const int& round, const Demo::Player& player, const Demo::Info& demoInfo) {
    auto pOperating = this->pDemSys->GetCurrentOperatingDemoName();
    if (!pOperating)return;
    const auto& info = player.roundInfo.at(round);

    if (!this->showBekillEvent && info.killEvents.empty())return;
    ImGui::Separator();

    const auto& killEvents = info.killEvents;

    // ---------- 渲染击杀事件表 ----------
    auto t = MulNX::UI::RAIITable("KillEventsTable", { "操作","Tick","被击杀者","助攻者","所用武器" });
    if (!t)return;
    MulNX::UI::SmartButton btn{round};
    for (size_t idx = 0; idx < killEvents.size(); ++idx) {
        const auto& ev = killEvents[idx];
        ImGui::TableNextRow();

        // 列0：单事件录制按钮
        ImGui::TableSetColumnIndex(0);
        if (btn.Next("录制")) {
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/Record/Make/Kill"_hash);
            rp->str1 = *pOperating;
            auto&& [uid, rRound, rIdx] = msg.Access<Steam64UID, size_t, size_t>();
            uid = ev.killerSteamId;
            rRound = round;
            rIdx = idx;
            this->PublishAsync(std::move(msg));
        }
        ImGui::SameLine();
        if (btn.Next("受害者视角")) {
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/Record/Make/Death"_hash);
            rp->str1 = *pOperating;
            auto&& [uid, rRound] = msg.Access<Steam64UID, size_t>();
            uid = ev.victimSteamId;
            rRound = round;
            this->PublishAsync(std::move(msg));
        }

        // 列1-4：事件信息
        ImGui::TableSetColumnIndex(1); ImGui::Text("%d", ev.tick);
        ImGui::TableSetColumnIndex(2); ImGui::Text("%s", demoInfo.GetPlayerName(ev.victimSteamId).c_str());
        ImGui::TableSetColumnIndex(3); ImGui::Text("%s", demoInfo.GetPlayerName(ev.assisterSteamId).c_str());
        ImGui::TableSetColumnIndex(4); ImGui::Text("%s", ev.weaponName.c_str());
    }

    // 被击杀事件
    if (this->showBekillEvent && info.Bekilled.has_value()) {
        const auto& ev = info.Bekilled.value();
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        if (btn.Next("录制被击杀")) {
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/Record/Make/Death"_hash);
            rp->str1 = *pOperating;
            auto&& [uid, rRound] = msg.Access<Steam64UID, size_t>();
            uid = ev.victimSteamId;
            rRound = round;
            this->PublishAsync(std::move(msg));
        }

        ImGui::TableSetColumnIndex(1); ImGui::Text("%d", ev.tick);
        ImGui::TableSetColumnIndex(2); ImGui::TextUnformatted(std::format("{} (由 {} 击杀)", demoInfo.GetPlayerName(ev.victimSteamId), demoInfo.GetPlayerName(ev.killerSteamId)).c_str());
        ImGui::TableSetColumnIndex(3); ImGui::Text("%s", demoInfo.GetPlayerName(ev.assisterSteamId).c_str());
        ImGui::TableSetColumnIndex(4); ImGui::Text("%s", ev.weaponName.c_str());
    }
}
bool RecordTaskMaker::Init() {
    this->pConfiger = this->FindModule<RecordTaskConfiger>("RecordTaskConfiger");
    this->UIRegisterCallback("UI.Demos", [this](auto uico, auto&&...) {return this->Window(uico);});
    return true;
}