#include "DemState.hpp"

bool DemState::Init() {
    (*this)
        .SubscribeAsync("Demo/SetPlaying")
        ;

    this->SendTask("Update", "DemoSys", [this]() {
        this->Update();
        return true;
        });

    return true;
}

void DemState::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Demo/SetPlaying"_hash: {
        const auto full = fs::path(msg.asp.get<MulNX::NetExt>()->str1);
        this->currentPlayingDemoName = std::make_shared<std::string>(full.stem().string());
        // 发送分析请求（包含完整路径）
        auto [ana, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/Analyze"_hash);
        rp->str1 = full.string();
        this->PublishAsync(std::move(ana));
        break;
    }
    }
}