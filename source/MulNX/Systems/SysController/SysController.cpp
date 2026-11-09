#include "SysController.hpp"
#include <MulNX/MulNX.hpp>
#include <MulNX/Systems/Debugger/Debugger.hpp>
#include <MulNX/Base/UI/UI.hpp>

void SysController::Window(MulNX::UICoordinator* uico) {
    auto w = MulNX::UI::RAIIWindow("控制");
    if (!w || !w.ShouldDraw())return;
    uico->CallbackCall("UI.MulNXControl"_hash, nullptr);
    if (ImGui::Button("尝试拉取所有模块信息")) {
        MulNX::Message Msg("ModuleManager/ModuleInfo/Request"_hash);
        this->PublishAsync(std::move(Msg));
    }
    
    ImGui::TextUnformatted("手动注入消息（异步消息），注意，使用此功能前必须明确知道目的，否则可能引发崩溃等");
    static std::string msgName{};
    ImGui::InputText("消息名", &msgName);
    static std::string str1{};
    ImGui::InputText("str1", &str1);
    static std::string str2{};
    ImGui::InputText("str2", &str2);
    static std::string str3{};
    ImGui::InputText("str3", &str3);

    if (ImGui::Button("注入到框架")) {
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>(MulNX::HashString(msgName));
        rp->str1 = str1;
        rp->str2 = str2;
        rp->str3 = str3;
        this->PublishAsync(std::move(msg));
    }
    if (ImGui::Button("注入错误")) {
        this->LogError("Test");
    }
}

bool SysController::Init() {
    (*this)
        .SubscribeAsync("ModuleManager/ModuleInfo/Response")
        ;

    this->SendUIRoot(this->GetName(), [this](auto uico, auto&&...) {return this->Window(uico);});

    this->SendTask("Main", "MulNXMain", [this]()->bool {
        this->Main();
        return true;
        });

    return true;
}

void SysController::ProcessMsg(MulNX::Message& Msg) {
    switch (Msg.type) {
    case "ModuleManager/ModuleInfo/Response"_hash: {
        auto* pInfo = Msg.asp.get<ModuleInfo>();
        this->LogInfo("检测到以下注册模块");
        for (auto& [name, handle] : pInfo->Info) {
            this->LogInfo(std::move(name));
        }
        break;
    }
    }
}
void SysController::Main() {
    this->Update();
}