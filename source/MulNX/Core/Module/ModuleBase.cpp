#include "ModuleBase.hpp"
#include <MulNX/Core/Core.hpp>
#include <MulNX/Core/ModuleManager/ModuleManager.hpp>
#include <MulNX/Systems/MessageManager/MessageChannel/MessageChannel.hpp>
#include <ranges>

MulNX::IModule* MulNX::ModuleBase::FindModule(const std::string& name) {
    return this->Core->ModuleManager()->FindModule(name);
}
// 初始化
bool MulNX::ModuleBase::EntryInit(MulNX::Core::Core* core) {
    this->Core = core;
    for (auto& init : this->preInits) {
        if (!init())return false;
    }
    this->preInits.clear();
    if (!this->Init()) {
        return false;
    }
    for (auto& init : this->postInits | std::views::reverse) {
        if (!init())return false;
    }
    this->postInits.clear();
    return true;
}
bool MulNX::ModuleBase::EntryDeinit() {
    for (auto& deinit : this->preDeinits) {
        if (!deinit())return false;
    }
    if (!this->Deinit()) {
        return false;
    }
    for (auto& deinit : this->postDeinits | std::views::reverse) {
        if (!deinit())return false;
    }
    return true;
}
void MulNX::ModuleBase::Update() {
    // 状态检查性协程恢复
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
    // 消息检查段
    // 注意这里，msg掌握潜在的asp对象，直到协程使用asp，仍保持有效，直到离开作用域，msg析构引起asp析构
    std::vector<MulNX::Message>msgs;
    while (true) {
        MulNX::Message msg;
        if (!this->msgChannel->PullMessage(msg))break;
        msgs.push_back(std::move(msg));
    }
    for (auto& msg : msgs) {
        this->ProcessMsg(msg);// 高优先级
        auto it = this->msgWaiters.find(msg.type);
        if (it == this->msgWaiters.end())continue;
        // 移动以防止迭代器失效，这里收到消息移动所有等待者
        auto waiters = std::move(it->second);
        for (auto& [check, h] : waiters) {
            check(&msg);
            h.resume();//  由于模块代码内聚，这里应当是不出现已经处理的情况
        }
        continue;
    }
    // 处理 nullptr 条件（超时/取消）的 msg waiter
    toResume.clear();
    for (auto& [type, waiters] : this->msgWaiters) {
        for (auto it = waiters.begin(); it != waiters.end(); ) {
            if (it->first(nullptr)) {
                toResume.push_back(it->second);
                it = waiters.erase(it);
            }
            else {
                ++it;
            }
        }
    }
    // 清掉空 key
    std::erase_if(this->msgWaiters, [](auto& kv) { return kv.second.empty(); });
    for (auto h : toResume) {
        h.resume();
    }
}