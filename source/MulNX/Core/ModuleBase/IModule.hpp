#pragma once
#include <MulNX/Common/Message.hpp>

namespace MulNX {
    class IModule {
        IModule(const IModule&) = delete;
        IModule(IModule&&) = delete;
        IModule& operator=(const IModule&) = delete;
        IModule& operator=(IModule&&) = delete;
    protected:
        virtual bool Init() = 0;
        // 消息处理函数，只需处理即可，消息会由入口点释放
        virtual void ProcessMsg(MulNX::Message& Msg) {};
    public:
        IModule() = default;
        virtual void Deinit() {};
        virtual ~IModule() = default;
    };
}