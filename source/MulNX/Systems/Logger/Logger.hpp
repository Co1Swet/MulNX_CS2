#pragma once

#include <MulNX/Core/ModuleBase/ModuleBase.hpp>
#include <MulNXThirdParty/queue/concurrentqueue.h>
#include <fstream>

namespace MulNX {
    class Log {
    public:
        enum class Level {
            Info,
            Succ,
            Warning,
            Error
        };

        ModuleBase* pModule = nullptr;
        Level level;
        std::string raw{};
        int64_t timestamp_us = 0; // 微秒，Unix epoch
    };
        
    class Logger final :public MulNX::ModuleBase {
    private:
        std::filesystem::path logPath{};
        std::ofstream target{};

        std::string lineFmt{};
        std::string kInfo{};
        std::string kSucc{};
        std::string kWarning{};
        std::string kError{};

        std::string PraseLevel(Log::Level level);
    public:
        moodycamel::ConcurrentQueue<Log>logs{};
        bool Init();
        void Log();
    };
}