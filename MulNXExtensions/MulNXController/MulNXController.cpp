#include "MulNXController.hpp"

#include <MulNX/MulNX.hpp>
#include <MulNX/Systems/Debugger/Debugger.hpp>
#include <MulNX/Base/UI/UI.hpp>

bool MulNXController::UINodeFunc(MulNX::UINode* ThisNode) {
    static bool debugMode = this->GlobalVars->DebugMode;
    if (ImGui::Checkbox("调试模式（Debug Mode），提供更多功能，但可能影响性能和稳定性", &debugMode)) {
        this->GlobalVars->DebugMode = debugMode;
    }
    MulNX::UI::Checkbox("调试器窗口", this->IDebugger->showWindow);
    ImGui::Checkbox("当有错误信息时弹出调试器", &this->IDebugger->ShowWhenError);
    ImGui::Checkbox("自动滚动到最新消息", &this->IDebugger->AutoScroll);
    static int MaxDebugMsgs = 1000;
    ImGui::Text("设置最大消息数量（至少100）:");
    ImGui::SameLine();
    ImGui::InputInt("##最大消息数量", &MaxDebugMsgs);
    ImGui::SameLine();
    if (ImGui::Button("应用")) {
        MulNX::Message Msg("Debugger/SetMaxInfoCount"_hash);
        Msg.p1.low<int>() = MaxDebugMsgs;
        this->ISys().PublishAsync(std::move(Msg));
    }
    if (ImGui::Button("尝试拉取所有模块信息")) {
        MulNX::Message Msg("ModuleManager/ModuleInfo/Request"_hash);
        this->ISys().PublishAsync(std::move(Msg));
    }
    static std::string msg{};
    ImGui::InputText("手动注入消息", &msg);
    if (ImGui::Button("注入到框架")) {
        this->ISys().PublishAsync(MulNX::HashString(msg));
        this->ISys().PublishSync(MulNX::HashString(msg));
        msg.clear();
    }
    ImGui::SameLine();
    if (ImGui::Button("注入到游戏")) {
        this->ISys().AsyncCommand(std::move(msg));
        msg.clear();
    }
    return true;
}

bool MulNXController::Init() {
    this->ISys()
        .SubscribeAsync("ModuleManager/ModuleInfo/Response");
    this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {return this->UINodeFunc(node);});
    this->SendTask("MulNXMain", [this]()->bool {
        this->Main();
        return true;
        });

    return true;
}

void MulNXController::ProcessMsg(MulNX::Message& Msg) {
    switch (Msg.type) {
    case "ModuleManager/ModuleInfo/Response"_hash: {
        auto* pInfo = Msg.asp.get<ModuleInfo>();
        this->ISys().LogInfo("检测到以下注册模块");
        for (auto& [Name, Handle] : pInfo->Info) {
            this->ISys().LogInfo(Name);
        }
    }
    }
}
void MulNXController::Main() {
    this->Update();
}