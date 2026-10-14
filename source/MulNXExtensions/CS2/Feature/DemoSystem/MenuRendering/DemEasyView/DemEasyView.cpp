#include "DemEasyView.hpp"

void DemEasyView::Window() {
    auto w = MulNX::UI::RAIIWindow("快速生成");
    if (!w.ShouldDraw())return;
    std::shared_lock lock(this->smutex);

    auto uid = this->localUid.load(std::memory_order_acquire);
    if (uid == 0) try {
        auto pCtrl = this->CS2->client.dwLocalPlayerController();
        if (!pCtrl)return;
        auto steamid = MulNX::MRead(pCtrl->m_steamID());
        auto name = MulNX::Memory::ReadString(pCtrl->m_iszPlayerName()).value_or({});
        if (!name.empty()) {
            this->localUid.store(steamid, std::memory_order_release);
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/SetLocalSteam"_hash);
            rp->str1 = name;
            auto&& [rUid] = msg.Access<Steam64UID>();
            rUid = steamid;
            this->PublishAsync(std::move(msg));
        }
        return;
    }
    catch (const MulNX::Exception& e) {
        this->LogWarning(e);
        return;
    }

    ImGui::Text(std::format("您的Steam数字ID是：{}", uid).c_str());
    ImGui::Text(std::format("您的Steam账户名是：{}", this->localName).c_str());
    if (ImGui::Button("不是您？点我刷新")) {
        this->localUid.store(0, std::memory_order_release);
    }
    ImGui::SeparatorText("立刻生成");
}

bool DemEasyView::Init() {

    (*this)
        .SubscribeAsyncEx<Steam64UID>("Demo/SetLocalSteam")
        ;

    this->SendTask("Update", "DemoSys", [this]() {
        this->Update();
        return true;
        });

    this->UIRegisterCallback("UI.Demos", [this](auto&&...) {
        this->Window();
        });

    return true;
}

void DemEasyView::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Demo/SetLocalSteam"_hash: {
        auto&& [uid] = msg.Access<Steam64UID>();
        this->localUid = uid;
        std::unique_lock lock(this->smutex);
        this->localName = msg.asp.get<MulNX::NetExt>()->str1;
        break;
    }
    }
}