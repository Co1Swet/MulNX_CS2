#pragma once

#include <MulNX/Core/ModuleBase/ModuleBase.hpp>
#include <functional>

namespace MulNX {
    namespace Core {
        // 核心启动器基类，定义核心启动的基本接口
        class CoreStarterBase :public MulNX::ModuleBase {
        public:
            // 初始化Core的所有系统组件
            bool SystemInit(MulNX::Core::Core* pCore);
            // 注册主绘制函数
            void CreateMainDraw();
            // 核心完全初始化后回调
            std::function<void()>InitEndCall = nullptr;
            // 激活系统，以开始工作
            void CloseSystem();
        };
    }
}