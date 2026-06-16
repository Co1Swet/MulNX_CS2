#pragma once
#include "IModule.hpp"
#include <MulNX/Common/coroutine.hpp>
#include <MulNX/Common/Message.hpp>
#include <MulNX/Base/MulNXHandle/MulNXHandle.hpp>
#include <MulNX/Systems/Logger/LogMixin.hpp>
#include <shared_mutex>

namespace MulNX {
    class MessageChannel;
    class ModuleComponents{
    public:
        // 便捷窗口显示标志
        std::atomic<bool> showWindow = false;
        std::atomic<bool>runFlag1 = false;
        std::atomic<bool>runFlag2 = false;
        std::shared_mutex smutex;

        MulNXHandle HModule;        
        MulNX::MessageChannel* MainMsgChannel;
        std::string ModuleName{};
    };
    class ModuleBase :public IModule, public ModuleComponents {
    private:
        // 协程：状态
        struct AwaitCondition;
        // 等待容器
        std::vector<AwaitCondition*> conditionWaiters;
        // 等待对象
        struct AwaitCondition {
            ModuleBase* pModuleBase;
            std::coroutine_handle<> h;
            std::function<bool()> condition;
            AwaitCondition(ModuleBase* pModuleBase, std::function<bool()>&& cond) :
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
    protected:
        // 自用更新入口
        void Update();  
        auto WaitUntil(std::function<bool()>&& condition) { return AwaitCondition(this, std::move(condition)); }
        auto WaitMsg(MulNX::MsgType type) { return AwaitMessage(this, type); }
    public:
        ~ModuleBase();
        // 设置模块名称
        void SetName(std::string&& Name) { this->ModuleName = std::move(Name); }
        std::string GetName()const { return this->ModuleName; };
        // 初始化入口
        bool EntryInit(MulNX::Core::Core* core);
    };
}