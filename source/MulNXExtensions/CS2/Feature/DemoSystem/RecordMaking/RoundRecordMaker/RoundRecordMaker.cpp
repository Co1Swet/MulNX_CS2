#include "RoundRecordMaker.hpp"
#include <Feature/DemoSystem/RecordTaskConfiger/RecordTaskConfiger.hpp>

bool RoundRecordMaker::Init() {
    this->pConfiger = this->FindModule<RecordTaskConfiger>("RecordTaskConfiger");

    (*this)
        .SubscribeAsyncEx<Steam64UID, size_t, size_t>("Demo/Record/Make/Kill")
        .SubscribeAsyncEx<Steam64UID, size_t>("Demo/Record/Make/RoundKill")
        .SubscribeAsyncEx<Steam64UID, size_t>("Demo/Record/Make/Death")
        .SubscribeAsyncEx<Steam64UID, size_t>("Demo/Record/Make/FullRound")
        .SubscribeAsync<int>("Demo/Record/Make/Flush")
        ;

    this->SendTask("Update", "DemoSys", [this]() {
        this->Update();
        return true;
        });

    return true;
}

void RoundRecordMaker::PublishRecordTask(Demo::RecordTask&& rTask)const {
    this->LogInfo(std::format("准备录制任务：{}", rTask.desc));
    auto [msg, rp] = MulNX::Message::Create<Demo::RecordTask>("Demo/Record/Enqueue"_hash);
    *rp = std::move(rTask);
    this->PublishAsync(std::move(msg));
}

void RoundRecordMaker::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Demo/Record/Make/Flush"_hash: {
        MulNX::Message commit(msg);
        commit.type = "Demo/Record/Commit"_hash;
        this->PublishAsync(std::move(commit));
        break;
    }
    case "Demo/Record/Make/Kill"_hash: {
        auto* pExt = msg.asp.get<MulNX::NetExt>();
        auto&& [uid, round, idx] = msg.Access<Steam64UID, size_t, size_t>();

        auto pDemoInfo = this->pDemSys->GetDemoInfo(pExt->str1);
        const auto& info = pDemoInfo->players.at(uid).roundInfo.at(round);
        const auto& ev = info.killEvents[idx];

        Demo::RecordTask rTask{};
        rTask.demoName = pExt->str1;
        rTask.uid = ev.killerSteamId;
        rTask.tickStart = ev.tick - this->pConfiger->preRecordTicks;
        rTask.tickEnd = ev.tick + this->pConfiger->postRecordTicks;
        rTask.roundNumber = static_cast<int>(round);
        rTask.roundKill = info.killEvents.size();
        rTask.killIndex = idx;
        rTask.desc = std::format("玩家 {} 击杀了 玩家 {}，录制从 {} tick到 {} tick",
            pDemoInfo->GetPlayerName(ev.killerSteamId),
            pDemoInfo->GetPlayerName(ev.victimSteamId),
            rTask.tickStart, rTask.tickEnd);
        rTask.fileNameNoExt = std::format("{}_{}_r{}_kill_{}", pExt->str1, uid, round, idx);
        this->PublishRecordTask(std::move(rTask));
        break;
    }
    case "Demo/Record/Make/RoundKill"_hash: {
        try {
            auto* pExt = msg.asp.get<MulNX::NetExt>();
            auto&& [uid, round] = msg.Access<Steam64UID, size_t>();

            auto pDemoInfo = this->pDemSys->GetDemoInfo(pExt->str1);
            const auto& info = pDemoInfo->players.at(uid).roundInfo.at(round);

            for (int i = 0;i < info.killEvents.size();++i) {
                const auto& ev = info.killEvents[i];
                Demo::RecordTask rTask{};
                rTask.demoName = pExt->str1;
                rTask.uid = ev.killerSteamId;
                rTask.tickStart = ev.tick - this->pConfiger->preRecordTicks;
                rTask.tickEnd = ev.tick + this->pConfiger->postRecordTicks;
                rTask.roundNumber = static_cast<int>(round);
                rTask.roundKill = info.killEvents.size();
                rTask.killIndex = i;
                rTask.desc = std::format("玩家 {} 击杀了 玩家 {}，录制从 {} tick到 {} tick",
                    pDemoInfo->GetPlayerName(ev.killerSteamId),
                    pDemoInfo->GetPlayerName(ev.victimSteamId),
                    rTask.tickStart, rTask.tickEnd);
                rTask.fileNameNoExt = std::format("{}_{}_r{}_kill_{}", pExt->str1, uid, round, i);
                this->PublishRecordTask(std::move(rTask));
            }
        }
        catch (...) {
            this->LogInfo("无有效数据，忽略一条请求");
        }
        break;
    }
    case "Demo/Record/Make/Death"_hash: {
        auto* pExt = msg.asp.get<MulNX::NetExt>();
        auto&& [uid, round] = msg.Access<Steam64UID, size_t>();
        auto pDemoInfo = this->pDemSys->GetDemoInfo(pExt->str1);
        const auto& info = pDemoInfo->players.at(uid).roundInfo.at(round);
        if (!info.Bekilled)break;
        auto& ev = *info.Bekilled;

        Demo::RecordTask rTask{};
        rTask.demoName = pExt->str1;
        rTask.uid = ev.victimSteamId;
        rTask.tickStart = ev.tick - this->pConfiger->preRecordTicksBekilled;
        rTask.tickEnd = ev.tick + this->pConfiger->postRecordTicksBekilled;
        rTask.roundNumber = static_cast<int>(round);
        rTask.desc = std::format("玩家 {} 被 玩家 {} 击杀，录制从 {} tick到 {} tick",
            pDemoInfo->GetPlayerName(ev.victimSteamId),
            pDemoInfo->GetPlayerName(ev.killerSteamId),
            rTask.tickStart, rTask.tickEnd);
        rTask.fileNameNoExt = std::format("{}_{}_r{}_death", pExt->str1, uid, round);
        this->PublishRecordTask(std::move(rTask));
        break;
    }
    case "Demo/Record/Make/FullRound"_hash: {
        auto* pExt = msg.asp.get<MulNX::NetExt>();
        auto&& [uid, round] = msg.Access<Steam64UID, size_t>();
        auto pDemoInfo = this->pDemSys->GetDemoInfo(pExt->str1);

        // 查找当前回合的元数据（startTick, endTick）
        auto itRound = std::find_if(pDemoInfo->rounds.begin(), pDemoInfo->rounds.end(),
            [&](const Demo::Round& r) { return r.number == round; });
        if (itRound == pDemoInfo->rounds.end()) {
            this->LogError(std::format("查找不到回合：{}", round));
            break;
        }

        // 计算起点：提前3秒（3 * 64 = 192 ticks）
        int startTickFull = std::max(0, itRound->freezeTimeEndTick - 192);
        // 计算终点
        int endTickFull = itRound->endTick;
        const auto& player = pDemoInfo->players.at(uid);
        if (player.roundInfo.contains(round)) {
            auto info = player.roundInfo.at(round);
            if (info.Bekilled.has_value()) {
                endTickFull = std::min(info.Bekilled->tick + 64, endTickFull);
            }
        }
        endTickFull += this->pConfiger->postTickRoundEnd;

        Demo::RecordTask rTask{};
        rTask.demoName = pExt->str1;
        rTask.uid = player.steamId;
        rTask.tickStart = startTickFull;
        rTask.tickEnd = endTickFull;
        rTask.roundNumber = static_cast<int>(round);
        rTask.desc = std::format("玩家 {} 第 {} 回合整段录制（{} ~ {} tick）",
            player.name, round, rTask.tickStart, rTask.tickEnd);
        rTask.fileNameNoExt = std::format("{}_{}_r{}_full", pExt->str1, uid, round);
        this->PublishRecordTask(std::move(rTask));
        break;
    }
    }
}