#include "DemoSystem.hpp"

void DemoSystem::Window(MulNX::UICoordinator* uico) {
    auto w = MulNX::UI::RAIIWindow("Demo系统", this->showWindow);
    if (!w) return;
    uico->CallbackCall("UI.Demos"_hash, nullptr);
    if (!w.ShouldDraw())return;
    uico->CallbackCall("UI.Demo.Main"_hash, nullptr);
}

bool DemoSystem::Init() {
    (*this)
        .SubscribeAsync("Demo/Play")
        .SubscribeAsync("Demo/SetPlaying")
        .SubscribeAsync("Demo/Info/Load")
        .SubscribeAsync("Demo/SetOperating")
        ;

    this->UIRegisterCallback("UI.Advanced", [this](auto&&...) {
        MulNX::UI::Checkbox("Demo系统", this->showWindow);
        });

    this->SendUIRoot(this->GetName(), [this](auto uico, auto&&...) {
        return this->Window(uico);
        });

    this->SendTask("Update", "DemoSys", [this]() {
        this->Update();
        return true;
        });

    this->Path()->CreateKey("kGAMEDEMOS", "csgo", [this](auto* pm) {
        return true;
        });
    this->Path()->KeyBindStatic("kGAMEDEMOS", this->CS2Paths->root / "game");

    this->PublishAsync("Demo/Refresh"_hash);

    return true;
}

void DemoSystem::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Demo/Play"_hash: {
        auto& key = msg.asp.get<MulNX::NetExt>()->str1;
        auto& demoName = msg.asp.get<MulNX::NetExt>()->str2;
        auto path = this->Path()->PathGetFromKey(key) / (demoName + ".dem");
        this->AsyncCommand(std::format("playdemo \"{}\"", path.string()));
        break;
    }
    case "Demo/SetPlaying"_hash: {
        const auto& key = msg.asp.get<MulNX::NetExt>()->str1;
        const auto& demoName = msg.asp.get<MulNX::NetExt>()->str2;
        this->currentPlayingDemoName = std::make_shared<std::string>(demoName);
        // 发送分析请求
        auto [ana, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/Analyze"_hash);
        rp->str1 = key;
        rp->str2 = demoName;
        this->PublishAsync(std::move(ana));
        break;
    }
    case "Demo/Info/Load"_hash: {
        std::unique_lock lock(this->smutex);
        auto demoInfo = *msg.asp.get<Demo::Info>();
        auto kDir = demoInfo.kDir;
        auto demoName = demoInfo.demoFileName;
        this->demos[demoInfo.kDir][demoName] = std::make_shared<Demo::Info>(std::move(demoInfo));
        auto [notify, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/Info/Notify"_hash);
        auto&& [succ] = notify.Access<int>();
        succ = 0;
        rp->str1 = std::move(kDir);
        rp->str2 = std::move(demoName);
        this->PublishAsync(std::move(notify));
        break;
    }
    case "Demo/SetOperating"_hash: {
        std::unique_lock lock(this->smutex);
        this->currentOperatingDemoName =
            std::make_shared<std::string>(msg.asp.get<MulNX::NetExt>()->str1);
        break;
    }
    }
}

std::shared_ptr<const std::string> DemoSystem::GetCurrentOperatingDemoName()const {
    return this->currentOperatingDemoName.load(std::memory_order_acquire);
}
std::shared_ptr<const std::string> DemoSystem::GetCurrentPlayingDemoName()const {
    return this->currentPlayingDemoName.load(std::memory_order_acquire);
}
std::shared_ptr<const Demo::Info> DemoSystem::GetDemoInfo(const std::string& demoName)const {
    std::shared_lock lock(this->smutex);
    for (const auto& [kDir, demoMap] : this->demos) {
        auto it = demoMap.find(demoName);
        if (it != demoMap.end()) {
            return it->second;
        }
    }
    return nullptr;
}
std::shared_ptr<const Demo::Info> DemoSystem::GetCurOpDemoInfo()const {
    std::shared_lock lock(this->smutex);
    auto pCurOp = this->currentOperatingDemoName.load(std::memory_order_acquire);
    if (!pCurOp)return nullptr;
    for (const auto& [kDir, demoMap] : this->demos) {
        auto it = demoMap.find(*pCurOp);
        if (it != demoMap.end()) {
            return it->second;
        }
    }
    return nullptr;
}