#include "DemoRecordMaker.hpp"
#include <Feature/DemoSystem/RecordTaskConfiger/RecordTaskConfiger.hpp>

bool DemoRecordMaker::Init() {
    this->pConfiger = this->FindModule<RecordTaskConfiger>("RecordTaskConfiger");

    (*this)
        .SubscribeAsyncEx<Steam64UID>("Demo/Record/Make/FullRoundAll")
        .SubscribeAsyncEx<Steam64UID>("Demo/Record/Make/RoundKillAll")
        .SubscribeAsync<int>("Demo/Record/Commit/Demo")
        ;

    this->SendTask("Update", "DemoSys", [this]() {
        this->Update();
        return true;
        });
    
    return true;
}

void DemoRecordMaker::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Demo/Record/Make/FullRoundAll"_hash: {
        auto* pExt = msg.asp.get<MulNX::NetExt>();
        auto&& [uid] = msg.Access<Steam64UID>();
        auto pDemoInfo = this->pDemSys->GetDemoInfo(pExt->str1);
        this->LogInfo(std::format("为玩家 {} 准备所有回合录制", pDemoInfo->players.at(uid).name));
        for (int i = pDemoInfo->rounds.front().number;i <= pDemoInfo->rounds.back().number;++i) {
            auto [one, p] = MulNX::Message::Create<MulNX::NetExt>("Demo/Record/Make/FullRound"_hash);
            p->str1 = pDemoInfo->demoFileName;
            auto&& [rUid, rRound] = one.Access<Steam64UID, size_t>();
            rUid = uid;
            rRound = i;
            this->PublishAsync(std::move(one));
        }
        break;
    }
    case "Demo/Record/Make/RoundKillAll"_hash: {
        auto* pExt = msg.asp.get<MulNX::NetExt>();
        auto&& [uid] = msg.Access<Steam64UID>();
        auto pDemoInfo = this->pDemSys->GetDemoInfo(pExt->str1);
        this->LogInfo(std::format("为玩家 {} 准备所有击杀录制", pDemoInfo->players.at(uid).name));
        for (int i = pDemoInfo->rounds.front().number;i <= pDemoInfo->rounds.back().number;++i) {
            auto [one, p] = MulNX::Message::Create<MulNX::NetExt>("Demo/Record/Make/RoundKill"_hash);
            p->str1 = pDemoInfo->demoFileName;
            auto&& [rUid, rRound] = one.Access<Steam64UID, size_t>();
            rUid = uid;
            rRound = i;
            this->PublishAsync(std::move(one));
        }
        break;
    }
    case "Demo/Record/Commit/Demo"_hash: { // 依赖无锁队列的特性，当所有其它任务处理投入之后，最后投入，以推动所有任务完成后一次性提交
        MulNX::Message flush(msg);
        flush.type = "Demo/Record/Commit/Round"_hash;
        this->PublishAsync(std::move(flush));
        break;
    }
    }
}