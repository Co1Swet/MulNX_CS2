#pragma once
#include <MulNX/Core/Core.hpp>
#include <MulNX/Core/Module/Module.hpp>
#include <functional>

namespace MulNX {
    namespace Core {
        // 核心启动器基类，定义核心启动的基本接口
        class Driver :public MulNX::Module<Driver> {
        public:
            // 初始化Core的所有系统组件
            bool Init()override;
            // 注册主绘制函数
            void CreateMainDraw();
            // 激活系统，以开始工作
            void CloseSystem();
        };
    }
}