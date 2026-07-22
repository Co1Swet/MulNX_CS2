#pragma once
#include "ModuleCoroutine.hpp"
#include <MulNX/Base/MulNXHandle/MulNXHandle.hpp>
#include <shared_mutex>

namespace MulNX {
    class MessageChannel;
    class ModuleComponents{
    public:
        // 便捷窗口显示标志
        std::atomic<bool> showWindow = false;
        std::atomic<bool> runFlag1 = false;
        std::atomic<bool> runFlag2 = false;
        std::shared_mutex smutex;

        MulNXHandle HModule;        
        MulNX::MessageChannel* MainMsgChannel;
        std::string ModuleName{};
    };
    class ModuleBase :public ModuleCoroutine, public ModuleComponents {
    protected:
        // 自用更新入口
        void Update();  
    public:
        ~ModuleBase();
        // 设置模块名称
        void SetName(std::string&& Name) { this->ModuleName = std::move(Name); }
        std::string GetName()const { return this->ModuleName; };
        // 初始化入口
        bool EntryInit(MulNX::Core::Core* core);
    };
}