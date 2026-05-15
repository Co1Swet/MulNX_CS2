#pragma once

#include <MulNX/Config/Config.hpp>
#include <MulNX/Systems/InputSystem/Key/Key.hpp>
#include <filesystem>

namespace MulNX {
    class C_ISys {
        friend ModuleBase;
        C_ISys() = delete;
        ModuleBase* pModuleBase = nullptr;
        C_ISys(ModuleBase* pModuleBase) {
            this->pModuleBase = pModuleBase;
        }
    public:
        void LogInfo(const std::string& Msg);
        void LogSucc(const std::string& Msg);
        void LogWarning(const std::string& Msg);
        void LogError(const std::string& Msg);

        // 提示级别
        void LogLine();

        C_ISys& SubscribeAsync(const std::string& msgType);
        void PublishAsync(MulNX::Message&& msg);
        void PublishAsync(MulNX::MsgType msgType);

        C_ISys& SubscribeSync(const std::string& msgType, MulNX::SyncMsgCallback&& handle);
        void PublishSync(MulNX::Message& msg);
        void PublishSync(MulNX::MsgType msgType);

        void AsyncCommand(std::string&& cmd);

        std::filesystem::path PathGet(const std::string& Target);
        MulNX::PathManager* PathManager();

        std::optional<MulNX::KeyCheckPack> GetButton(const std::string& name);
    };
}