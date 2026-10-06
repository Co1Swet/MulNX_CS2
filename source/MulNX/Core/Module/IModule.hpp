#pragma once
#include <MulNX/Config/Config.hpp>
#include <MulNX/Common/Message.hpp>

namespace MulNX {
    class IModule {
        IModule(const IModule&) = delete;
        IModule(IModule&&) = delete;
        IModule& operator=(const IModule&) = delete;
        IModule& operator=(IModule&&) = delete;
    protected:
        virtual bool Init() = 0;
        virtual void ProcessMsg(MulNX::Message& msg) {};
        // 模块需要自行保证此函数的线程安全性，此函数常常用于抛出信号停止自己的线程
        // 如有资源释放尽量走析构函数
        virtual void Deinit() {};
    public:
        IModule() = default;
        virtual ~IModule() = default;
    };
}