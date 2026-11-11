#pragma once
#include "ModuleCoroutine.hpp"
#include <MulNX/Common/MulNXHandle.hpp>
#include <shared_mutex>
#include <string_view>

namespace MulNX {
    class MessageChannel;
    class ModuleComponents{
    protected:
        mutable std::atomic<bool> showWindow = false;
        mutable std::shared_mutex smutex;
        std::string moduleName{};
    public:
        MulNXHandle hModule;
        MulNX::MessageChannel* msgChannel;
        // 设置模块名称
        inline void SetName(std::string&& name) { this->moduleName = std::move(name); }
        inline std::string GetName()const { return this->moduleName; };
        inline std::string_view GetNameView()const noexcept { return this->moduleName; }
    };

    class ModuleBase :public ModuleCoroutine, public ModuleComponents {
        MulNX::IModule* FindModule(const std::string& name)const;
    protected:
        // 自用更新入口
        void Update();  
    public:
        // 初始化入口
        bool EntryInit(MulNX::Core::Core* core);
        bool EntryDeinit();

        template<typename T>
        inline T* FindModule(const std::string& name)const {
            return static_cast<T*>(this->FindModule(name));
        }
    };
}