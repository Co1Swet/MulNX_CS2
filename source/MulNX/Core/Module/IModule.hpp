#pragma once
#include <MulNX/Common/Message.hpp>
#include <MulNX/Core/Core.hpp>
#include <MulNX/Systems/I18nManager/I18n.hpp>

namespace MulNX {
    class IModule {
        IModule(const IModule&) = delete;
        IModule(IModule&&) = delete;
        IModule& operator=(const IModule&) = delete;
        IModule& operator=(IModule&&) = delete;
    protected:
        virtual bool Init() = 0;
        virtual void ProcessMsg(MulNX::Message& Msg) {};
    public:
        MulNX::Core::Core* Core = nullptr;
        std::unique_ptr<std::vector<std::function<bool()>>>delayInits = std::make_unique<std::vector<std::function<bool()>>>();

        IModule() = default;
        virtual void Deinit() {};
        virtual ~IModule() = default;

        IModule* FindModule(const std::string& name);
        template<typename T>
        T* FindModule(const std::string& Name) {
            return static_cast<T*>(this->FindModule(Name));
        }
    };
}