#pragma once
#include <MulNX/Common/Message.hpp>

namespace MulNX {
    class IModuleBase {
        IModuleBase(const IModuleBase&) = delete;
        IModuleBase(IModuleBase&&) = delete;
        IModuleBase& operator=(const IModuleBase&) = delete;
        IModuleBase& operator=(IModuleBase&&) = delete;
    protected:
        virtual bool Init() = 0;
        // 消息处理函数，只需处理即可，消息会由入口点释放
        virtual void ProcessMsg(MulNX::Message& Msg) {};
    public:
        IModuleBase() = default;
        virtual void Deinit() {};
        virtual ~IModuleBase() = default;
    };
}