#include <MulNXExtensions/Win32Starter/Win32Starter.hpp>
#include <MulNXExtensions/MulNXController/MulNXController.hpp>
#include <MulNXInjector/UIDocker/UIDocker.hpp>

MulNX::Core::Core* pCore = nullptr;

// Main code
int main(int, char**) {
    try {
        // 创建核心
        pCore = MulNX::Core::Core::Create("MulNXInjector");
        // 创建核心启动器
        auto* starter = pCore->CreateCoreStarter<Win32Starter>();
        // 手动创建的模块需要手动设置名称
        starter->SetName("Win32Starter");
        // 设置初始化完成回调
        starter->InitEndCall = [starter]() {
            starter->ISys().LogWarning(I18n("disclaimer"));
            };

        // 注册所有模块
        (*pCore->ModuleManager())
            .CreateSystemModules()// 创建所有系统模块，这是框架运行的基础
            // 管理
            .CreateModule<MulNXController>("MulNXController")
            .CreateModule<UIDocker>("UIDocker")
            ;

        // 启动核心
        pCore->Init();
        // 启动主循环
        starter->Run();
    }
    catch (std::exception& e) {
        MulNX::ErrorTerminate("在启动时发生异常！异常描述：" + std::string(e.what()));
    }
    catch (...) {
        MulNX::ErrorTerminate("在启动时发生未知异常！");
    }
    return 0;
}