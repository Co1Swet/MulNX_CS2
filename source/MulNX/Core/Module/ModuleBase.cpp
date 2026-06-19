#include "ModuleBase.hpp"
#include <MulNX/Core/Core.hpp>
#include <MulNX/Core/ModuleManager/ModuleManager.hpp>
#include <MulNX/Systems/Systems.hpp>
#include <ranges>

// 初始化
bool MulNX::ModuleBase::EntryInit(MulNX::Core::Core* core) {
    this->Core = core;
    for (const auto& init : *this->delayInits) {
        if (!init())return false;
    }
    this->delayInits.reset();
    if (!this->Init()) {
        return false;
    }
    for (const auto& init : *this->backInits | std::views::reverse) {
        if (!init())return false;
    }
    this->backInits.reset();
    return true;
}
void MulNX::ModuleBase::Update() {
    std::vector<AwaitCondition*> toResume;
    for (auto it = this->conditionWaiters.begin(); it != this->conditionWaiters.end(); ) {
        if ((*it)->condition()) {
            toResume.push_back(*it);
            it = this->conditionWaiters.erase(it);
        }
        else {
            ++it;
        }
    }
    for (auto& cw : toResume) {
        cw->h.resume();
    }
    MulNX::MessageChannel* Channel = this->MainMsgChannel;
    // 注意这里，msg掌握潜在的asp对象，直到协程使用asp，仍保持有效，直到离开作用域，msg析构引起asp析构
    std::vector<MulNX::Message>msgs;
    while (true) {
        MulNX::Message msg;
        if (!Channel->PullMessage(msg))break;
        msgs.push_back(std::move(msg));
    }
    for (auto& msg : msgs) {
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
        if (cw->h) {
            cw->h.destroy();
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