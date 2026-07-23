#pragma once
#include <MulNX/Config/Config.hpp>
#include <MulNX/Common/Message.hpp>
#include <MulNX/Common/Exception.hpp>
#include <MulNX/Systems/I18nManager/I18n.hpp>

namespace MulNX {
    namespace Core{
        class Core;
    }
    class UINode;
    class IModule {
        IModule(const IModule&) = delete;
        IModule(IModule&&) = delete;
        IModule& operator=(const IModule&) = delete;
        IModule& operator=(IModule&&) = delete;
    protected:
        virtual bool Init() = 0;
        virtual void ProcessMsg(MulNX::Message& Msg) {};
        // 模块需要自行保证此函数的线程安全性，此函数常常用于抛出信号停止自己的线程
        // 如有资源释放尽量走析构函数
        virtual void Deinit() {};
    public:
        MulNX::Core::Core* Core = nullptr;
        std::vector<std::function<bool()>>preInits{};
        std::vector<std::function<bool()>>postInits{};
        std::vector<std::function<bool()>>preDeinits{};
        std::vector<std::function<bool()>>postDeinits{};
        
        IModule() = default;
        virtual ~IModule() = default;
        
        IModule* FindModule(const std::string& name);
        template<typename T>
        T* FindModule(const std::string& Name) {
            return static_cast<T*>(this->FindModule(Name));
        }
    };
}