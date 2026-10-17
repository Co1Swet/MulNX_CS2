#include "DemEasyView.hpp"

void DemEasyView::Window() {
    auto w = MulNX::UI::RAIIWindow("快速生成");
    if (!w.ShouldDraw())return;
    std::shared_lock lock(this->smutex);

    // 自动探测
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
    // 状态显示
    ImGui::TextUnformatted(std::format("您的Steam数字ID是：{}", uid).c_str());
    ImGui::TextUnformatted(std::format("您的Steam账户名是：{}", this->localName).c_str());
    if (ImGui::CollapsingHeader("不是您？点我")) {
        if (ImGui::Button("自动更新")) {
            this->localUid.store(0, std::memory_order_release);
        }
        static ImU64 uidBuf = 0;
        ImGui::InputScalar("Steam数字ID", ImGuiDataType_U64, &uidBuf, nullptr, nullptr, "%llu");
        if (ImGui::Button("提交")) {
            this->localUid.store(uidBuf, std::memory_order_release);
        }
    }
    
    ImGui::SeparatorText("立刻生成");

    auto pDemo = this->pDemSys->GetCurOpDemoInfo();
    if (!pDemo) {
        ImGui::TextUnformatted("当前没有选中Demo！您需要先选择一个哦");
        return;
    }
    auto it = pDemo->players.find(this->localUid.load(std::memory_order_acquire));
    if (it == pDemo->players.end()) {
        ImGui::TextUnformatted("当前选中的Demo中没有您！");
        return;
    }
    const auto& player = it->second;
    for (const auto& [idx, round] : player.roundInfo) {
        auto count = round.killEvents.size();
        if (count < 3)continue;
        ImGui::SeparatorText(std::format("第 {} 回合", idx).c_str());
        if (ImGui::Button(std::format("{}杀  一键生成##{}", count, idx).c_str())) {
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/Record/Make/RoundKill"_hash);
            rp->str1 = pDemo->demoFileName;
            auto&& [rUid, rRound] = msg.Access<Steam64UID, size_t>();
            rUid = uid;
            rRound = idx;
            this->PublishAsync(std::move(msg));
        }
    }
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