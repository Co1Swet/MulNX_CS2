#pragma once
#include <MulNX/Core/Core.hpp>
#include <MulNX/Core/Module/Module.hpp>
#include <functional>

namespace MulNX {
    namespace Core {
        // 核心驱动，定义MulNX启停流程
        class Driver :public MulNX::Module<Driver> {
            bool Init()override;
        public:
            Driver();
            // 调用并阻塞当前线程直到关闭
            void WaitEnd();
        };
    }
}