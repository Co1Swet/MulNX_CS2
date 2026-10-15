#include "DemState.hpp"

void DemState::Menu()const {
    for (const auto& [name, demo] : this->demos) {
        if (ImGui::Button(name.c_str())) {
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/SetOperating"_hash);
            rp->str1 = name;
            this->PublishAsync(std::move(msg));
        }
        ImGui::SameLine();
    }
    ImGui::NewLine();
}

bool DemState::Init() {
    (*this)
        .SubscribeAsync("Demo/SetPlaying")
        .SubscribeAsync("Demo/InfoLoad")
        .SubscribeAsync("Demo/SetOperating")
        ;

    this->UIRegisterCallback("UI.Demo.Choose", [this](auto&&...) {
        this->Menu();
        });
    
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
    case "Demo/InfoLoad"_hash: {
        std::unique_lock lock(this->smutex);
        auto demoInfo = *msg.asp.get<Demo::Info>();
        this->demos[demoInfo.demoFileName] = std::make_shared<Demo::Info>(demoInfo);
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

std::shared_ptr<const Demo::Info> DemState::GetDemoInfo(const std::string& demoName)const {
    std::shared_lock lock(this->smutex);
    auto it = this->demos.find(demoName);
    if (it == this->demos.end())return nullptr;
    return it->second;
}