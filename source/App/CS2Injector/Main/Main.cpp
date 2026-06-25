#include <MulNXExtensions/Win32Starter/Win32Starter.hpp>
#include <MulNXExtensions/MulNXController/MulNXController.hpp>
#include <MulNXExtensions/DLLInjectHelper/DLLInjectHelper.hpp>
#include <MulNXExtensions/CS2BootLoader/CS2BootLoader.hpp>
#include <App/CS2Injector/UIDocker/UIDocker.hpp>

// Main code
int main(int, char**) {
    try {
        // 创建核心
        auto core = MulNX::Core::Core::Create("CS2Injector");
        // 将模块句柄传递给核心，以便后续使用
        core->hMyOriginModule = GetModuleHandleW(NULL);
        // 注册所有模块
        (*core->ModuleManager())
            .CreateSystemModules()// 创建所有系统模块，这是框架运行的基础
            .CreateModule<Win32Starter>("Win32Starter")
            // 管理
            .CreateModule<DLLInjectHelper>("DLLInjectHelper")
            .CreateModule<CS2BootLoader>("CS2BootLoader")
            .CreateModule<MulNXController>("MulNXController")
            .CreateModule<UIDocker>("UIDocker")
            ;

        auto pStarter = core->ModuleManager()->FindModule<Win32Starter>("Win32Starter");

        // 启动核心
        core->EntryInit(core.get());
        // 启动主循环
        pStarter->Run();
        core->Driver()->WaitEnd();
        return 0;
    }
    catch (std::exception& e) {
        MulNX::ErrorTerminate("在启动时发生异常！异常描述：" + std::string(e.what()));
    }
    catch (...) {
        MulNX::ErrorTerminate("在启动时发生未知异常！");
    }
    return 0;
}