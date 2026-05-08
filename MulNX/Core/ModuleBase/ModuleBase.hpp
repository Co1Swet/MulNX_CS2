#pragma once

#include <MulNX/Common/coroutine.hpp>
#include <MulNX/Common/Message.hpp>
#include <MulNX/Base/MulNXHandle/MulNXHandle.hpp>
#include "ISys/ISys.hpp"
#include <shared_mutex>
#include <thread>
#include <functional>

namespace MulNX {
    class ModuleBase {
        friend MulNX::Core::Core;
        friend C_ISys;
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

        // 等待句柄
        struct MessageWaiter {
            std::coroutine_handle<> handle;
            MulNX::Message* result = nullptr;  // 用于存放拉取到的消息
        };
        // 等待容器
        std::unordered_map<size_t, std::vector<MessageWaiter>> msgWaiters;
        // 等待对象
        struct AwaitMessage {
            ModuleBase* pModuleBase;
            MulNX::MsgType type;
            MulNX::Message* result = nullptr;  // 恢复后从这取消息
            AwaitMessage(ModuleBase* pModuleBase, MulNX::MsgType type) :
                pModuleBase(pModuleBase), type(type) {}
            bool await_ready() { return false; }  // 总是挂起，或检查缓存
            void await_suspend(std::coroutine_handle<> h) {
                // 注册到模块
                pModuleBase->msgWaiters[type].push_back({ h, result });
            }
            MulNX::Message& await_resume() {
                return *result;
            }
        };

        MulNX::MessageManager* pMsgManager = nullptr;
        MulNX::PathManager* pPathManager = nullptr;
    protected:
        // 父模块句柄
        MulNXHandle hParent{};
        // 模块名称，唯一标识
        std::string ModuleName{};
        MulNX::Core::Core* Core = nullptr;
        MulNX::GlobalVars* GlobalVars = nullptr;
        // 线程运行状态
        std::atomic<bool>Running = false;
        // 线程执行间隔，默认以100Hz基准执行
        std::atomic<int> MyThreadDelta = 10;
    public:
        MulNX::InputSystem* pInputSystem = nullptr;
        // 组件句柄
        MulNXHandle HModule;
        Debugger* IDebugger = nullptr;
        MulNX::MessageChannel* MainMsgChannel = nullptr;
        std::shared_mutex smutex;
    public:
        // 删除不需要的构造函数
        ModuleBase(const ModuleBase&) = delete;
        ModuleBase(ModuleBase&&) = delete;
        ModuleBase& operator=(const ModuleBase&) = delete;
        ModuleBase& operator=(ModuleBase&&) = delete;
        ModuleBase() = default;
        // 虚析构函数确保正确调用析构函数
        virtual ~ModuleBase() = default;
    private:
        // 虚函数要求：

        // 初始化
        virtual bool Init() = 0;

        // 消息处理函数，只需处理即可，消息会由入口点释放
        virtual void ProcessMsg(MulNX::Message& Msg) {};
    public:
        // 基础初始化
        bool BaseInit(MulNX::Core::Core* core);
        // 初始化入口
        bool EntryInit();
    protected:
        // 消息处理入口
        void EntryProcessMsg();
        // 通过任意函数，发送一个UI节点
        bool SendUINode(std::string&& name, std::function<void(MulNX::UINode*)>&& func);
        void SendTask(std::string&& workerName, std::function<bool()>&& task);
        auto WaitUntil(std::function<bool()>&& condition) {
            return AwaitCondition(this, std::move(condition));
        }
        auto WaitMsg(MulNX::MsgType type) {
            return AwaitMessage(this, type);
        }
    public:
        // 设置模块名称
        bool SetName(std::string&& Name);
        std::string GetName()const;
        // 得到核心指针
        MulNX::Core::Core* GetCore()const { return this->Core; }
        // 设置父模块句柄
        void SetParent(MulNXHandle hModule);
        // 是否有父模块
        bool HasParent();
        // 便捷窗口显示标志
        std::atomic<bool> ShowWindow = false;
        // 模块时间控制接口
        void SetMyThreadDelta(int Delta);

        // 系统服务包装器(原则上是protected权限)
        C_ISys ISys();
    };
}