#include "DemWrapMaker.hpp"
#include <Feature/DemoSystem/DemRecord/RecordTaskConfiger/RecordTaskConfiger.hpp>

bool DemWrapMaker::Init() {
    this->pConfiger = this->FindModule<RecordTaskConfiger>("RecordTaskConfiger");

    (*this)
        .SubscribeAsyncEx<Steam64UID>("Demo/Record/Make/FullRoundAll")
        ;

    this->SendTask("Update", "DemoSys", [this]() {
        this->Update();
        return true;
        });
    
    return true;
}

void DemWrapMaker::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Demo/Record/Make/FullRoundAll"_hash: {
        auto* pExt = msg.asp.get<MulNX::NetExt>();
        auto&& [uid] = msg.Access<Steam64UID>();
        auto pDemoInfo = this->pDemState->GetDemoInfo(pExt->str1);

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
    }
}