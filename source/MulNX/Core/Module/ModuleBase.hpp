#pragma once
#include "ModuleCoroutine.hpp"
#include <shared_mutex>
#include <queue>

namespace MulNX {
    class MessageChannel;

    class ModuleBase :public ModuleCoroutine {
        std::queue<MulNX::Message> bufferMsgs{};
        MulNXHandle hModule;
    protected:
        mutable std::shared_mutex smutex;
    public:
        MulNX::MessageChannel* msgChannel;

    private:
        MulNX::IModule* FindModule(const std::string& name)const;
        MulNXHandle GetHandle()const override final;
        void SetHandle(MulNXHandle handle)override final;
    protected:
        void Update()override final;
    public:
        bool DrainBootstraps(MulNX::Core::Core* core)override final;
        bool EntryInit()override final;
        bool EntryDeinit()override final;
        
        template<typename T>
        inline T* FindModule()const {
            return static_cast<T*>(this->FindModule(std::string(T::GetLeafName())));
        }
    };
}