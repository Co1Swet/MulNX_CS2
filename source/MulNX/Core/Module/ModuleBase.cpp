#include "ModuleBase.hpp"
#include <MulNX/Core/Core.hpp>
#include <MulNX/Systems/MessageManager/MessageChannel/MessageChannel.hpp>
#include <ranges>

// 初始化
bool MulNX::ModuleBase::EntryInit(MulNX::Core::Core* core) {
    this->Core = core;
    for (const auto& init : this->preInits) {
        if (!init())return false;
    }
    this->preInits.clear();
    if (!this->Init()) {
        return false;
    }
    for (const auto& init : this->postInits | std::views::reverse) {
        if (!init())return false;
    }
    this->postInits.clear();
    return true;
}
bool MulNX::ModuleBase::EntryDeinit() {
    for (const auto& deinit : this->preDeinits) {
        if (!deinit())return false;
    }
    this->Deinit();
    for (const auto& deinit : this->postDeinits | std::views::reverse) {
        if (!deinit())return false;
    }
    return true;
}
void MulNX::ModuleBase::Update() {
    std::vector<std::coroutine_handle<>> toResume;
    for (auto it = this->conditionWaiters.begin(); it != this->conditionWaiters.end(); ) {
        if (it->first()) {
            toResume.push_back(it->second);
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

        for (auto& [check, h] : waiters) {
            check(&msg);
            h.resume();//  由于模块代码内聚，这里应当是不出现已经处理的情况
        }
        continue;
    }
    for (auto& [type, waiters] : this->msgWaiters) {
        for (auto it = waiters.begin();it != waiters.end();) {
            if (it->first(nullptr)) {
                it->second.resume();
                it = waiters.erase(it);
            }
            else {
                ++it;
            }
        }
    }
    return;
}