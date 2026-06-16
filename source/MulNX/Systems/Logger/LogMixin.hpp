#pragma once
#include <MulNX/Core/Module/IModule.hpp>

namespace MulNX {
    class ModuleBase;
    class Logger;
    class Log {
    public:
        enum class Level {
            Info,
            Succ,
            Warning,
            Error
        };

        const ModuleBase* pModule = nullptr;
        Level level;
        int64_t timestamp_us = 0; // 微秒，Unix epoch
        std::string raw;
    };

    template<typename T>
    class LogMixin {
    private:
        Logger* logger = nullptr;
    public:
        T* This() { return static_cast<T*>(this); }
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
        void LogInfo(std::string&& msg) {
            MulNX::Log log;
            log.pModule = This();
            log.timestamp_us = MulNX::ToUnixUs(std::chrono::system_clock::now());
            log.level = MulNX::Log::Level::Info;
            log.raw = std::move(msg);

            this->logger->logs.enqueue(std::move(log));
        }
        void LogError(std::string&& msg) {
            MulNX::Log log;
            log.pModule = This();
            log.timestamp_us = MulNX::ToUnixUs(std::chrono::system_clock::now());
            log.level = MulNX::Log::Level::Error;
            log.raw = std::move(msg);

            this->logger->logs.enqueue(std::move(log));
        }
        void LogWarning(std::string&& msg) {
            MulNX::Log log;
            log.pModule = This();
            log.timestamp_us = MulNX::ToUnixUs(std::chrono::system_clock::now());
            log.level = MulNX::Log::Level::Warning;
            log.raw = std::move(msg);

            this->logger->logs.enqueue(std::move(log));
        }
        void LogLine() {
            this->LogInfo("+------------------------------------------------+");
        }
    };
}