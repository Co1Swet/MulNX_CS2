#include <CS2OBTool/UIDocker/UIDocker.hpp>
#include <MulNXExtensions/CS2/CameraSystem/CamSysExt.hpp>
#include <MulNXExtensions/CS2/MulNXCS2Ext.hpp>
#include <MulNXExtensions/VirtualUser/VirtualUser.hpp>
#include <MulNXExtensions/MulNXController/MulNXController.hpp>
#include <MulNXExtensions/WebSocketManager/WebSocketManager.hpp>
#include <MulNXExtensions/MediaRemoter/MediaRemoter.hpp>
#include <MulNXExtensions/ShortcutManager/ShortcutManager.hpp>

MulNX::Core::Core* pCore = nullptr;

DWORD MulNX_CS2_Start(void*) {
    try {
        // 创建核心
        pCore = MulNX::Core::Core::Create("CS2OBTool");
        // 创建核心启动器
        auto* starter = pCore->CreateCoreStarter<HookManager>();
        // 手动创建的模块需要手动设置名称
        starter->SetName("HookManager");
        // 设置初始化完成回调
        starter->InitEndCall = [starter]() {
            starter->ISys().LogWarning(I18n("disclaimer"));
            if (MulNXInfo::IsDebugVersion) {
                auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/PlayAndAnalyze"_hash);
                rp->str1 = "111";
                starter->ISys().PublishAsync(std::move(msg));
                std::thread([]() {
                    MessageBoxW(NULL, L"MulNX 注入成功！", L"MulNX", MB_OK | MB_ICONINFORMATION);
                    }).detach();
            }
            };

        // 注册所有模块
        (*pCore->ModuleManager())
            .CreateSystemModules()// 创建所有系统模块，这是框架运行的基础
            .CreateModule<CSController>("CSController")
            .CreateModule<TimeController>("TimeController")
            .CreateModule<ViewController>("ViewController")
            .CreateModule<MulNX::ShaderCompiler>("ShaderCompiler")
            .CreateModule<MulNX::GraphicsManager>("GraphicsManager")
            .CreateModule<WebSocketManager>("WebSocketManager")
            .CreateModule<ShortcutManager>("ShortcutManager")
            // 摄像机系统
            .CreateModule<CameraSystem>("CameraSystem")
            .CreateModule<WorkspaceManager>("WorkspaceManager")
            .CreateModule<ProjectManager>("ProjectManager")
            .CreateModule<SolutionManager>("SolutionManager")
            .CreateModule<ElementManager>("ElementManager")
            // CS2
            .CreateModule<HookEntitySystem>("HookEntitySystem")
            .CreateModule<AdvancedViewController>("AdvancedViewController")
            .CreateModule<FreeCameraController>("FreeCameraController")
            .CreateModule<PlayerHub>("PlayerHub")
            .CreateModule<PlayerFlashController>("PlayerFlashController")
            .CreateModule<NameController>("NameController")
            .CreateModule<GlowController>("GlowController")
            .CreateModule<SmokeController>("SmokeController")
            .CreateModule<ObserverController>("ObserverController")
            .CreateModule<ProjectileTracker>("ProjectileTracker")
            .CreateModule<DeathMsgController>("DeathMsgController")
            .CreateModule<ESPController>("ESPController")
            .CreateModule<SkinController>("SkinController")
            //.CreateModule<TeamIDController>("TeamIDController")
            .CreateModule<POVFixer>("POVFixer")
            // Demos
            .CreateModule<DemoHelper>("DemoHelper")
            .CreateModule<DemoAnalyzer>("DemoAnalyzer")
            .CreateModule<DemoRecorder>("DemoRecorder")
            .CreateModule<DemoSystem>("DemoSystem")
            // 较为上层
            .CreateModule<MiniMap>("MiniMap")
            .CreateModule<VirtualUser>("VirtualUser")
            .CreateModule<GameCfgManager>("GameCfgManager")
            .CreateModule<GameSettingsManager>("GameSettingsManager")
            .CreateModule<ConsoleManager>("ConsoleManager")
            .CreateModule<MediaRemoter>("MediaRemoter")
            // 管理
            .CreateModule<MulNXController>("MulNXController")
            .CreateModule<UIDocker>("UIDocker")
            ;

        // 启动核心
        pCore->Init();
    }
    catch (std::exception& e) {
        MulNX::ErrorTerminate("在启动时发生异常！异常描述：" + std::string(e.what()));
    }
    catch (...) {
        MulNX::ErrorTerminate("在启动时发生未知异常！");
    }
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: {
        HANDLE hThread = CreateThread(NULL, 0, MulNX_CS2_Start, NULL, 0, NULL);
        // 这里不需要等待线程结束，因为它会在完成初始化后自动退出，然后等待进程结束时被操作系统清理
        break;
    }
    case DLL_THREAD_ATTACH: {
        break;
    }
    case DLL_THREAD_DETACH: {
        break;
    }
    case DLL_PROCESS_DETACH: {
        pCore->Close();
        break;
    }
    default: {
        break;
    }
    }
    return TRUE;
}