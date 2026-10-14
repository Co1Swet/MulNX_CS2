#include "DemEasyView.hpp"

bool DemEasyView::Init() {
    this->SubscribeSync("Hook/Present/First", [this](auto&&...) {
        try {
            auto pCtrl = this->CS2->client.dwLocalPlayerController();
            this->localUid = MulNX::MRead(pCtrl->m_steamID());
            this->LogSucc(std::format("探测本地玩家成功：{}", this->localUid.load()));
        }
        catch (const MulNX::Exception& e) {
            this->LogError(e);
            this->LogError("识别本地玩家失败");
        }
        });

    (*this)
        .SubscribeAsync<Steam64UID>("Demo/SetLocalSteamUID")
        ;

    this->SendTask("Update", "DemoSys", [this]() {
        this->Update();
        return true;
        });

    return true;
}

void DemEasyView::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Demo/SetLocalSteamUID"_hash: {
        auto&& [uid] = msg.Access<Steam64UID>();
        this->localUid = uid;
        break;
    }
    }
}