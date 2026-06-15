#pragma once
#include <MulNX/Core/ModuleBase/IModule.hpp>

namespace MulNX {
    class Log {
    public:
        enum class Level {
            Info,
            Succ,
            Warning,
            Error
        };

        IModule* pModule = nullptr;
        Level level;
        int64_t timestamp_us = 0; // 微秒，Unix epoch
        std::string raw;

    };

    class Logger;

    template<typename Derived>
    class LogMixin {
    private:
        Logger* logger = nullptr;
    public:
        IModule* This() { return static_cast<IModule*>(static_cast<Derived*>(this)); }
        LogMixin() {
            This()->delayInits->push_back([this]() {
                this->logger = static_cast<Logger*>(This()->FindModule("Logger"));
                return true;
                });
        }

        void LogSucc(std::string&& msg) {
            MulNX::Log log;
            log.pModule = This();
            log.timestamp_us = MulNX::ToUnixUs(std::chrono::system_clock::now());
            log.level = MulNX::Log::Level::Succ;
            log.raw = std::move(msg);

            this->logger->logs.enqueue(std::move(log));
        }
    };
}