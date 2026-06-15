#pragma once

#include <MulNX/Core/ModuleBase/ModuleBase.hpp>
#include <MulNXThirdParty/queue/concurrentqueue.h>
#include <fstream>
#include <Windows.h>

namespace MulNX {
        
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
        void test() { MessageBoxW(NULL, L"test", L"test", MB_OK); }
    };
}