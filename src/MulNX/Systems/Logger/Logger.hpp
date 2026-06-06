#pragma once

#include <MulNX/Core/ModuleBase/ModuleBase.hpp>
#include <MulNXThirdParty/queue/concurrentqueue.h>
#include <fstream>

namespace MulNX {
    class Logger final :public MulNX::ModuleBase {
    private:
        std::filesystem::path logPath{};
        std::ofstream target{};
    public:
        moodycamel::ConcurrentQueue<std::string>logs{};
        bool Init();
        void Log();
    };
}