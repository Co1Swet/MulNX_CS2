#pragma once
#include "IModule.hpp"
#include <MulNX/Common/coroutine.hpp>

namespace MulNX {
    class ModuleCoroutine: public IModule {
    protected:
        // 协程：状态
        struct AwaitCondition;
        // 等待容器
        std::vector<AwaitCondition*> conditionWaiters;
        // 等待对象
        struct AwaitCondition {
            ModuleCoroutine* pModuleBase;
            std::coroutine_handle<> h;
            std::function<bool()> condition;
            AwaitCondition(ModuleCoroutine* pModuleBase, std::function<bool()>&& cond) :
                pModuleBase(pModuleBase), condition(std::move(cond)) {}
            bool await_ready() {
                return condition();   // 若已满足，不挂起
            }
            void await_suspend(std::coroutine_handle<> h) {
                // 把句柄和条件函数打包，注册到模块的条件容器
                this->h = h;
                this->pModuleBase->conditionWaiters.push_back(this);
                // 悬空返回，协程挂起
            }
            void await_resume() {}
        };
        // 协程：消息
        struct AwaitMessage;
        // 等待容器
        std::unordered_map<size_t, std::vector<AwaitMessage*>> msgWaiters;
        // 等待对象
        struct AwaitMessage {
            ModuleCoroutine* pModuleBase;
            MulNX::MsgType type;
            std::coroutine_handle<> h;
            MulNX::Message* result = nullptr;  // 恢复后从这取消息
            AwaitMessage(ModuleCoroutine* pModuleBase, MulNX::MsgType type) :
                pModuleBase(pModuleBase), type(type) {}
            bool await_ready() { return false; }  // 总是挂起，或检查缓存
            void await_suspend(std::coroutine_handle<> h) {
                // 注册到模块
                this->h = h;
                this->pModuleBase->msgWaiters[type].push_back(this);
            }
            MulNX::Message& await_resume() {
                return *this->result;
            }
        };
    };
}