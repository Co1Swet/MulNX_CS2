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

    this->PublishAsync("Demo/Refresh"_hash);

    return true;
}

void DemoSystem::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Demo/Play"_hash: {
        auto& path = msg.asp.get<MulNX::NetExt>()->str1;
        this->AsyncCommand(std::format("playdemo \"{}\"", path));
        break;
    }
    case "Demo/SetPlaying"_hash: {
        const auto full = fs::path(msg.asp.get<MulNX::NetExt>()->str1);
        this->currentPlayingDemoName = std::make_shared<std::string>(full.stem().string());
        // 发送分析请求（包含完整路径）
        auto [ana, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/Analyze"_hash);
        rp->str1 = full.string();
        this->PublishAsync(std::move(ana));
        break;
    }
    case "Demo/Info/Load"_hash: {
        std::unique_lock lock(this->smutex);
        auto demoInfo = *msg.asp.get<Demo::Info>();
        auto name = demoInfo.demoFileName;
        this->demos[name] = std::make_shared<Demo::Info>(std::move(demoInfo));
        auto [notify, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/Info/Notify"_hash);
        rp->str1 = std::move(name);
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
    auto it = this->demos.find(demoName);
    if (it == this->demos.end())return nullptr;
    return it->second;
}
std::shared_ptr<const Demo::Info> DemoSystem::GetCurOpDemoInfo()const {
    std::shared_lock lock(this->smutex);
    auto pCurOp = this->currentOperatingDemoName.load(std::memory_order_acquire);
    if (!pCurOp)return nullptr;
    auto it = this->demos.find(*pCurOp);
    if (it == this->demos.end())return nullptr;
    return it->second;
}