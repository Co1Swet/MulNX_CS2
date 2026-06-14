#include "ModuleBase.hpp"
#include <MulNX/Core/Core.hpp>
#include <MulNX/Core/ModuleManager/ModuleManager.hpp>
#include <MulNX/Systems/MessageManager/MessageManager.hpp>
#include <MulNX/Systems/I18nManager/I18nManager.hpp>
#include <MulNX/Systems/Debugger/Debugger.hpp>
#include <MulNX/Systems/GlobalVars/GlobalVars.hpp>
#include <MulNX/Systems/InputSystem/InputSystem.hpp>
#include <MulNX/Systems/PathManager/PathManager.hpp>
#include <MulNX/Systems/ShortcutManager/ShortcutManager.hpp>
#include <MulNX/Systems/Logger/Logger.hpp>

bool MulNX::ModuleBase::SetName(std::string&& Name) {
    this->ModuleName = std::move(Name);
    return true;
}
std::string MulNX::ModuleBase::GetName()const {
    return this->ModuleName;
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
        this->pShortcutManager = moduleManager->FindModule<MulNX::ShortcutManager>("ShortcutManager");
        this->pLogger = moduleManager->FindModule<MulNX::Logger>("Logger");
        this->MainMsgChannel = this->pMsgManager->GetMessageChannel(this->pMsgManager->CreateMessageChannel());
    }
    catch (...) {
        return false;
    }

    return true;
}
bool MulNX::ModuleBase::EntryInit() {
    for (const auto& init : *this->delayInits) {
        if (!init())return false;
    }
    this->delayInits.reset();
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
        if (it == this->msgWaiters.end())continue;

        auto waiters = std::move(it->second);

        for (auto& w : waiters) {
            w->result = &msg;
            w->h.resume();//  由于模块代码内聚，这里应当是不出现已经处理的情况
        }
        continue;
    }
    return;
}
// 在 ModuleBase 类声明中添加
MulNX::ModuleBase::~ModuleBase() {
    // 销毁所有挂起的条件等待协程
    for (auto& cw : conditionWaiters) {
        if (cw.handle) {
            cw.handle.destroy();
        }
    }
    // 销毁所有挂起的消息等待协程
    for (auto& [type, vec] : msgWaiters) {
        for (auto& w : vec) {
            if (w->h) {
                w->h.destroy();
            }
        }
    }
}