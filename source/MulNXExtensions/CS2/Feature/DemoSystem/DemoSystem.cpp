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
        auto pExt = msg.asp.get<MulNX::NetExt>();
        const auto& key = pExt->str1;
        const auto& demoName = pExt->str2;
        this->currentPlayingDemo = std::make_shared<std::pair<std::string, std::string>>(key, demoName);
        // 发送分析请求
        auto [ana, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/Analyze"_hash);
        rp->str1 = key;
        rp->str2 = demoName;
        this->PublishAsync(std::move(ana));
        break;
    }
    case "Demo/Info/Load"_hash: {
        this->OnInfoLoad(msg);
        break;
    }
    case "Demo/SetOperating"_hash: {
        auto pExt = msg.asp.get<MulNX::NetExt>();
        const auto& key = pExt->str1;
        const auto& demoName = pExt->str2;
        this->currentOperatingDemo =
            std::make_shared<std::pair<std::string, std::string>>(key, demoName);
        break;
    }
    }
}
void DemoSystem::OnInfoLoad(MulNX::Message& msg) {
    std::unique_lock lock(this->smutex);
    auto pDemoInfo = msg.asp.get<Demo::Info>();
    if(!pDemoInfo){
        this->LogError("Demo/Info/Load 消息中缺少 Demo::Info");
        return;
    }
    auto demoInfo = std::move(*pDemoInfo);

    auto [notify, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/Info/Notify"_hash);
    auto kDir = demoInfo.kDir;
    auto demoName = demoInfo.demoFileName;
    try {
        this->demos[kDir][demoName] = std::make_shared<Demo::Info>(std::move(demoInfo));
        auto&& [succ] = notify.Access<int>();
        succ = 0;
        rp->str1 = std::move(kDir);
        rp->str2 = std::move(demoName);
        this->PublishAsync(std::move(notify));
    }
    catch (const std::exception& e) {
        this->LogError(std::format("处理 Demo/Info/Load 消息时发生异常: {}", e.what()));
        auto&& [succ] = notify.Access<int>();
        succ = 1;
        rp->str1 = std::move(kDir);
        rp->str2 = std::move(demoName);
        this->PublishAsync(std::move(notify));
    }
    catch (...) {
        this->LogError("处理 Demo/Info/Load 消息时发生未知异常");
        auto&& [succ] = notify.Access<int>();
        succ = 1;
        rp->str1 = std::move(kDir);
        rp->str2 = std::move(demoName);
        this->PublishAsync(std::move(notify));
    }
}

std::shared_ptr<std::pair<std::string, std::string>> DemoSystem::GetCurrentOperatingDemo()const {
    return this->currentOperatingDemo.load(std::memory_order_acquire);
}
std::shared_ptr<std::pair<std::string, std::string>> DemoSystem::GetCurrentPlayingDemo()const {
    return this->currentPlayingDemo.load(std::memory_order_acquire);
}
std::shared_ptr<const Demo::Info> DemoSystem::GetDemoInfo(const std::string& kDir, const std::string& demoName)const {
    std::shared_lock lock(this->smutex);
    auto it = this->demos.find(kDir);
    if (it == this->demos.end())return nullptr;
    auto demoIt = it->second.find(demoName);
    if (demoIt == it->second.end())return nullptr;
    return demoIt->second;
}
std::shared_ptr<const Demo::Info> DemoSystem::GetCurOpDemoInfo()const {
    std::shared_lock lock(this->smutex);
    auto pCurOp = this->currentOperatingDemo.load(std::memory_order_acquire);
    if (!pCurOp)return nullptr;
    auto it = this->demos.find(pCurOp->first);
    if (it == this->demos.end())return nullptr;
    auto demoIt = it->second.find(pCurOp->second);
    if (demoIt == it->second.end())return nullptr;
    return demoIt->second;
}