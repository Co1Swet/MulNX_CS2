#include "DemOneKillMaker.hpp"
#include <Feature/DemoSystem/DemRecord/RecordTaskConfiger/RecordTaskConfiger.hpp>

bool DemOneKillMaker::Init() {
    this->pConfiger = this->FindModule<RecordTaskConfiger>("RecordTaskConfiger");

    (*this)
        .SubscribeAsync<Steam64UID, size_t, size_t>("Demo/Record/Make/Kill")
        .SubscribeAsync<Steam64UID, size_t>("Demo/Record/Make/Death")
        ;

    this->SendTask("Update", "DemoSys", [this]() {
        this->Update();
        return true;
        });

    return true;
}

void DemOneKillMaker::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Demo/Record/Make/Kill"_hash: {
        auto* pExt = msg.asp.get<MulNX::NetExt>();
        auto&& [uid, round, idx] = msg.Access<Steam64UID, size_t, size_t>();

        auto pDemoInfo = this->pDemState->GetDemoInfo(pExt->str1);
        const auto& info = pDemoInfo->players.at(uid).roundInfo.at(round);
        const auto& ev = info.killEvents[idx];

        RecordTask rTask;
        rTask.uid = ev.killerSteamId;
        rTask.tickStart = ev.tick - this->pConfiger->preRecordTicks;
        rTask.tickEnd = ev.tick + this->pConfiger->postRecordTicks;
        rTask.desc = std::format("玩家 {} 击杀了 玩家 {}，录制从 {} tick到 {} tick",
            pDemoInfo->GetPlayerName(ev.killerSteamId),
            pDemoInfo->GetPlayerName(ev.victimSteamId),
            rTask.tickStart, rTask.tickEnd);
        rTask.fileNameNoExt = std::format("{}_{}_{}_{}", pExt->str1, uid, round, idx);

        this->PublishAsync(MulNX::Message::Create<RecordTask>("Demo/Record/Enqueue"_hash, std::move(rTask)).first);
        break;
    }
    case "Demo/Record/Make/Death"_hash: {
        auto* pExt = msg.asp.get<MulNX::NetExt>();
        auto&& [uid, round] = msg.Access<Steam64UID, size_t>();
        auto pDemoInfo = this->pDemState->GetDemoInfo(pExt->str1);
        const auto& info = pDemoInfo->players.at(uid).roundInfo.at(round);
        if (!info.Bekilled)break;
        auto& ev = *info.Bekilled;
        RecordTask rTask;
        rTask.uid = ev.victimSteamId;
        rTask.tickStart = ev.tick - this->pConfiger->preRecordTicksBekilled;
        rTask.tickEnd = ev.tick + this->pConfiger->postRecordTicksBekilled;
        rTask.desc = std::format("玩家 {} 被 玩家 {} 击杀，录制从 {} tick到 {} tick",
            pDemoInfo->GetPlayerName(ev.victimSteamId),
            pDemoInfo->GetPlayerName(ev.killerSteamId),
            rTask.tickStart, rTask.tickEnd);
        rTask.fileNameNoExt = std::format("{}_{}_{}_death", pExt->str1, uid, round);

        this->PublishAsync(MulNX::Message::Create<RecordTask>("Demo/Record/Enqueue"_hash, std::move(rTask)).first);
        break;
    }
    }
}