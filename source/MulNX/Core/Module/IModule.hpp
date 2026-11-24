#pragma once
#include <MulNX/Config/Config.hpp>
#include <MulNX/Common/MulNXHandle.hpp>
#include <MulNX/Common/Message.hpp>

namespace MulNX {
    namespace Core {
        class Core;
    }

    class IModule {
        IModule(const IModule&) = delete;
        IModule(IModule&&) = delete;
        IModule& operator=(const IModule&) = delete;
        IModule& operator=(IModule&&) = delete;
    protected:
        virtual bool Init() = 0;
        virtual void ProcessMsg(MulNX::Message& msg) {}
        // 模块需要自行保证此函数的线程安全性，此函数常常用于抛出信号停止自己的线程
        // 如有资源释放尽量走析构函数
        virtual bool Deinit() { return true; }
    public:
        IModule() = default;
        virtual ~IModule() = default;

        virtual bool DrainBootstraps(MulNX::Core::Core* core) = 0;
        virtual bool EntryInit() = 0;
        virtual bool EntryDeinit() = 0;

        virtual MulNXHandle GetHandle()const = 0;
        virtual void SetHandle(MulNXHandle handle) = 0;

        virtual void Update() = 0;

        virtual const char* GetName()const = 0;
        virtual const std::string_view& GetNameView()const = 0;
    };
}