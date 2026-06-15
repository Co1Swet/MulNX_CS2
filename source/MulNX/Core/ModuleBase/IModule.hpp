#pragma once
#include <MulNX/Common/Message.hpp>
#include <MulNX/Core/Core.hpp>

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
        // 延迟初始化任务
        std::unique_ptr<std::vector<std::function<bool()>>>delayInits = std::make_unique<std::vector<std::function<bool()>>>();
        IModule() = default;
        virtual void Deinit() {};
        virtual ~IModule() = default;

        std::string ModuleName{};
        MulNX::Core::Core* Core = nullptr;
        // 设置模块名称
        void SetName(std::string&& Name) { this->ModuleName = std::move(Name); }
        std::string GetName()const { return this->ModuleName; };

        IModule* FindModule(const std::string& name);
    };
}