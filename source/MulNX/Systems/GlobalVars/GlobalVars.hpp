#pragma once
#include <MulNX/Core/Module/Module.hpp>

namespace MulNX {
    class GlobalVars final :public Module<GlobalVars> {
		void PublishTickAll();
		void Tick();
	public:
        // 框架系统是否就绪
        std::atomic<bool>SystemReady = false;
        // 调试功能设置
        // 调试模式下提供更多功能，但可能影响性能和稳定性
        std::atomic<bool>DebugMode = false;
        // 核心心跳
		std::atomic<uint32_t>CoreTick = 0;
        // 是否让部分高危错误直接引发崩溃
        std::atomic<bool>DangerousErrorShouldTerminate = false;

        std::atomic<int>windowX = 1920;
        std::atomic<int>windowY = 1080;
        std::atomic<int>renderX = 1920;
        std::atomic<int>renderY = 1080;

		bool Init()override;
		void Main();
	};
}