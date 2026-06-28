#pragma once
#include <MulNX/Core/Module/Module.hpp>
#include <MulNXThirdParty/queue/blockingconcurrentqueue.h>
#include <fstream>

namespace MulNX {
    class Logger final :public MulNX::Module<Logger> {
    private:
        std::filesystem::path logPath{};
        std::ofstream target{};

        std::string lineFmt{};
        std::string kInfo{};
        std::string kSucc{};
        std::string kWarning{};
        std::string kError{};

        std::string PraseLevel(Level level);
        MulNX::MsgType MakeMsgType(Level level);
    public:
        moodycamel::BlockingConcurrentQueue<LogEntry>logs{};
        bool Init();
        void Log();
    };
}