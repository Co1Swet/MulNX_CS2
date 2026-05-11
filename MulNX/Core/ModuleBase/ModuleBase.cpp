#include "ModuleBase.hpp"

#include <MulNX/Base/UI/UI.hpp>
#include <MulNX/Core/Core.hpp>
#include <MulNX/Core/ModuleManager/ModuleManager.hpp>
#include <MulNX/Systems/MessageManager/MessageManager.hpp>
#include <MulNX/Systems/UISystem/UISystem.hpp>
#include <MulNX/Systems/TaskSystem/TaskSystem.hpp>
#include <MulNX/Systems/I18nManager/I18nManager.hpp>

bool MulNX::ModuleBase::SetName(std::string&& Name) {
    this->ModuleName = std::move(Name);
    return true;
}
std::string MulNX::ModuleBase::GetName()const {
    return this->ModuleName;
}

// 模块自用
void MulNX::ModuleBase::SetMyThreadDelta(int Delta) {
    this->MyThreadDelta = Delta;
}

// 初始化
bool MulNX::ModuleBase::BaseInit(MulNX::Core::Core* core) {
    this->Core = core;
    try {
        auto* moduleManager = this->Core->ModuleManager();
        this->pMsgManager = moduleManager->FindModule<MulNX::MessageManager>("MessageManager");
        this->IDebugger = moduleManager->FindModule<MulNX::Debugger>("Debugger");
        this->GlobalVars = moduleManager->FindModule<MulNX::GlobalVars>("GlobalVars");
        this->pInputSystem = moduleManager->FindModule<MulNX::InputSystem>("InputSystem");
        this->pPathManager = moduleManager->FindModule<MulNX::PathManager>("PathManager");
        this->MainMsgChannel = this->pMsgManager->GetMessageChannel(this->pMsgManager->CreateMessageChannel());
    }
    catch (...) {
        return false;
    }

    return true;
}

bool MulNX::ModuleBase::SendUINode(std::string&& name, std::function<void(MulNX::UINode*)>&& func) {
    // 创建UI节点
    MulNX::UINode UINode = MulNX::UINode::Create(this);
    // 设置UI节点属性
    UINode.name = std::move(name);
    UINode.MyFunc = std::move(func);
    // 创建UI消息
    auto [msg, rp] = MulNX::Message::Create<MulNX::UINode>("UISystem/ModulePush"_hash, std::move(UINode));
    // 发送UI消息
    this->ISys().PublishAsync(std::move(msg));
    this->ISys().LogInfo(I18n("module.send_ui"));
    return true;
}
void MulNX::ModuleBase::SendTask(std::string&& workerName, std::function<bool()>&& task) {
    auto [msg, rp] = MulNX::Message::Create<MulNX::Task::RegistrationPacket>("Task/Create"_hash);
    rp->targetWorker = std::move(workerName);
    rp->task = std::move(task);
    this->ISys().PublishAsync(std::move(msg));
    this->ISys().LogInfo(I18n("module.send_task"));
}

bool MulNX::ModuleBase::EntryInit() {
    if (!this->Init()) {
        return false;
    }
    this->ISys().LogSucc(I18n("module.inited"));
    return true;
}
void MulNX::ModuleBase::Update() {
    std::vector<std::coroutine_handle<>> toResume;
    for (auto it = this->conditionWaiters.begin(); it != this->conditionWaiters.end(); ) {
        if (it->condition()) {
            toResume.push_back(std::move(it->handle));
            it = this->conditionWaiters.erase(it);
        }
        else {
            ++it;
        }
    }
    for (auto& h : toResume) {
        h.resume();
    }
    MulNX::MessageChannel* Channel = this->MainMsgChannel;
    MulNX::Message msg{};
    // 注意这里，msg掌握潜在的asp对象，直到协程使用asp，仍保持有效，直到离开作用域，msg析构引起asp析构
    while (Channel->PullMessage(msg)) {
        this->ProcessMsg(msg);// 高优先级
        auto it = this->msgWaiters.find(msg.type);
        if (it != this->msgWaiters.end()) {
            auto waiters = std::move(it->second);
            this->msgWaiters.erase(it);
            for (auto& w : waiters) {
                w.result = &msg;
                w.handle.resume();//  由于模块代码内聚，这里应当是不出现已经处理的情况
            }
        }
        continue;
    }
    return;
}