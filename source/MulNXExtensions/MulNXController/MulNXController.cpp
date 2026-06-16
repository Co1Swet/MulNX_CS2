#include "MulNXController.hpp"
#include <MulNX/MulNX.hpp>
#include <MulNX/Systems/Debugger/Debugger.hpp>
#include <MulNX/Base/UI/UI.hpp>

bool MulNXController::UINodeFunc(MulNX::UINode* node) {
    MulNX::UI::Checkbox("调试模式（Debug Mode），提供更多功能，但可能影响性能和稳定性", this->pGlobalVars->DebugMode);
    node->CallUINode("DeDebugger");
    if (ImGui::Button("尝试拉取所有模块信息")) {
        MulNX::Message Msg("ModuleManager/ModuleInfo/Request"_hash);
        this->PublishAsync(std::move(Msg));
    }
    static std::string msg{};
    ImGui::InputText("手动注入消息", &msg);
    if (ImGui::Button("注入到框架")) {
        this->PublishAsync(MulNX::HashString(msg));
        this->PublishSync(MulNX::HashString(msg));
        msg.clear();
    }
    return true;
}

bool MulNXController::Init() {
    (*this)
        .SubscribeAsync("ModuleManager/ModuleInfo/Response");
    this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {return this->UINodeFunc(node);});
    this->SendTask("Main", "MulNXMain", [this]()->bool {
        this->Main();
        return true;
        });

    return true;
}

void MulNXController::ProcessMsg(MulNX::Message& Msg) {
    switch (Msg.type) {
    case "ModuleManager/ModuleInfo/Response"_hash: {
        auto* pInfo = Msg.asp.get<ModuleInfo>();
        this->LogInfo("检测到以下注册模块");
        for (auto& [name, handle] : pInfo->Info) {
            this->LogInfo(std::move(name));
        }
    }
    }
}
void MulNXController::Main() {
    this->Update();
}