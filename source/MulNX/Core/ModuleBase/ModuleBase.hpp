#pragma once
#include "IModule.hpp"
#include <MulNX/Common/coroutine.hpp>
#include <MulNX/Common/Task.hpp>
#include <MulNX/Base/MulNXHandle/MulNXHandle.hpp>
#include "ISys/ISys.hpp"
#include <shared_mutex>
#include <thread>
#include <functional>
#include <MulNX/Systems/Logger/LogMixin.hpp>

namespace MulNX {
    class ModuleBase :public IModule {
        friend ISys;
        friend MulNX::Core::CoreStarterBase;
    private:
        // 协程：状态

        // 等待句柄
        struct ConditionWaiter {
            std::coroutine_handle<> handle;
            std::function<bool()> condition;
        };
        // 等待容器
        std::vector<ConditionWaiter> conditionWaiters;
        // 等待对象
        struct AwaitCondition {
            ModuleBase* pModuleBase;
            std::function<bool()> condition;
            AwaitCondition(ModuleBase* pModuleBase, std::function<bool()>&& cond) :
                pModuleBase(pModuleBase), condition(std::move(cond)) {}
            bool await_ready() {
                return condition();   // 若已满足，不挂起
            }
            void await_suspend(std::coroutine_handle<> h) {
                // 把句柄和条件函数打包，注册到模块的条件容器
                ConditionWaiter waiter;
                waiter.handle = h;
                waiter.condition = std::move(condition);
                pModuleBase->conditionWaiters.push_back(std::move(waiter));
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
            ModuleBase* pModuleBase;
            MulNX::MsgType type;
            std::coroutine_handle<> h;
            MulNX::Message* result = nullptr;  // 恢复后从这取消息
            AwaitMessage(ModuleBase* pModuleBase, MulNX::MsgType type) :
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

        MulNX::MessageManager* pMsgManager = nullptr;
        MulNX::PathManager* pPathManager = nullptr;
        MulNX::MessageChannel* MainMsgChannel = nullptr;
        MulNX::ShortcutManager* pShortcutManager = nullptr;
        MulNX::Logger* pLogger = nullptr;
    protected:
        MulNX::GlobalVars* GlobalVars = nullptr;
        Debugger* IDebugger = nullptr;
    public:
        MulNX::InputSystem* pInputSystem = nullptr;
        MulNX::Core::Core* Core = nullptr;
        std::atomic<bool>runFlag1 = false;
        std::atomic<bool>runFlag2 = false;
        MulNXHandle HModule;
        
        std::shared_mutex smutex;
    protected:
        auto WaitUntil(std::function<bool()>&& condition) {return AwaitCondition(this, std::move(condition));}
        auto WaitMsg(MulNX::MsgType type) {return AwaitMessage(this, type);}
        // 更新入口
        void Update();
    public:
        ~ModuleBase();
        // 基础初始化
        bool BaseInit(MulNX::Core::Core* core);
        // 初始化入口
        bool EntryInit();
        // 得到核心指针
        MulNX::Core::Core* GetCore()const { return this->Core; }
        // 便捷窗口显示标志
        std::atomic<bool> showWindow = false;

        // 系统服务包装器(原则上是protected权限)
        ISys ISys();
    };

    template <typename T>
    concept Module = std::derived_from<T, MulNX::ModuleBase>;
}