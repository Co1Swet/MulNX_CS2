#pragma once
#include <MulNX/Core/Core.hpp>
#include <MulNX/Core/Module/Module.hpp>
#include <functional>

namespace MulNX {
    namespace Core {
        // 核心驱动，定义MulNX启停流程
        class Driver :public MulNX::Module<Driver> {
            std::mutex endMtx;
            std::condition_variable cv;
            bool endFlag = false;   // 共享状态
            bool Init()override;
        public:
            Driver();
            // 调用并阻塞当前线程直到关闭
            void WaitEnd();
        };
    }
}