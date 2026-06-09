#pragma once
#include <MulNX/Config/Config.hpp>
#include <MulNX/Systems/InputSystem/Key/Key.hpp>
#include <filesystem>

namespace MulNX {
    class ModuleBase;
    class ISys {
        friend ModuleBase;
        ISys() = delete;
        ModuleBase* pModuleBase = nullptr;
        ISys(ModuleBase* pModuleBase) {
            this->pModuleBase = pModuleBase;
        }
    public:
        void LogInfo(const std::string& Msg);
        void LogSucc(const std::string& Msg);
        void LogWarning(const std::string& Msg);
        void LogError(const std::string& Msg);
        void LogLine(); // 提示级别打印一条线

        ISys& SubscribeAsync(const std::string& msgType);
        void PublishAsync(MulNX::Message&& msg);
        void PublishAsync(MulNX::MsgType msgType);

        ISys& SubscribeSync(const std::string& msgType, MulNX::SyncMsgCallback&& handle);
        void PublishSync(MulNX::Message& msg);
        void PublishSync(MulNX::MsgType msgType);

        // 通过任意函数，发送一个UI节点
        bool SendUINode(std::string&& name, std::function<void(MulNX::UINode*)>&& func);
        void SendTask(std::string&& name, std::string&& targetWorker, std::function<bool()>&& Do);

        void AsyncCommand(std::string&& cmd);

        std::filesystem::path PathGet(const std::string& Target);
        MulNX::PathManager* PathManager();

        std::optional<MulNX::KeyCheckPack> GetButton(const std::string& name);
    };
}