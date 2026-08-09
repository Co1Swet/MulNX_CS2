#pragma once
#include <MulNX/Core/Module/IModule.hpp>

namespace MulNX {
    class ModuleBase;
    class Logger;
    enum class Level {
        Info,
        Succ,
        Warning,
        Error
    };
    class LogEntry {
    public:
        const ModuleBase* pModule = nullptr;
        std::source_location where;
        Level level;
        int64_t timestamp_us = 0; // 微秒，Unix epoch
        std::string raw;
    };

    template<typename T>
    class LogMixin {
        T* This() { return static_cast<T*>(this); }
        Logger* logger = nullptr;
    public:
        LogMixin() {
            This()->preInits.push_back([this]() {
                this->logger = static_cast<Logger*>(This()->FindModule("Logger"));
                return true;
                });
        }
        void Log(Level level, std::string&& msg, std::source_location where)const {
            MulNX::LogEntry log;
            log.pModule = static_cast<const T*>(this);
            log.where = where;
            log.timestamp_us = MulNX::ToUnixUs(std::chrono::system_clock::now());
            log.level = level;
            log.raw = std::move(msg);

            this->logger->logs.enqueue(std::move(log));
        }
        void LogSucc(std::string&& msg, std::source_location where = std::source_location::current())const {
            this->Log(Level::Succ, std::move(msg), where);
        }
        void LogInfo(std::string&& msg, std::source_location where = std::source_location::current())const {
            this->Log(Level::Info, std::move(msg), where);
        }
        void LogError(std::string&& msg, std::source_location where = std::source_location::current())const {
            this->Log(Level::Error, std::move(msg), where);
        }
        void LogWarning(std::string&& msg, std::source_location where = std::source_location::current())const {
            this->Log(Level::Warning, std::move(msg), where);
        }
        void LogInfo(const Exception& e)const {
            this->Log(Level::Info, e.what(), e.where);
        }
        void LogError(const Exception& e)const {
            this->Log(Level::Error, e.what(), e.where);
        }
        void LogWarning(const Exception& e)const {
            this->Log(Level::Warning, e.what(), e.where);
        }
        void LogLine()const {
            this->LogInfo("+------------------------------------------------+");
        }
    };
}