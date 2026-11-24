#pragma once
#include <MulNX/Core/Core.hpp>
#include <MulNX/Core/Module/Module.hpp>

namespace MulNX::Core {
    class Driver :public MulNX::Module<Driver> {
        std::mutex endMtx;
        std::condition_variable cv;
        bool endFlag = false;
        bool Init()override;
    public:
        Driver();
        bool BootstrapFramework();
        // 调用并阻塞当前线程直到关闭
        void WaitEnd();
    };
}