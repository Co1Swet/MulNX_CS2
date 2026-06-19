#include "DllMain.hpp"
#include <CS2OBTool/UIDocker/UIDocker.hpp>
#include <MulNXExtensions/DLLLoadDispatcher/DLLLoadDispatcher.hpp>
#include <MulNXExtensions/FileRedirector/FileRedirector.hpp>
#include <MulNXExtensions/CS2/CameraSystem/CamSysExt.hpp>
#include <MulNXExtensions/CS2/MulNXCS2Ext.hpp>
#include <MulNXExtensions/MulNXController/MulNXController.hpp>
#include <MulNXExtensions/WebSocketManager/WebSocketManager.hpp>
#include <MulNXExtensions/MediaSystem/Media.hpp>
#include <MulNXExtensions/TimeLiner/TimeLiner.hpp>

MulNX::Core::Core* pCore = nullptr;
HMODULE hOriginModule = nullptr;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        hOriginModule = hModule;
        break;
    case DLL_PROCESS_DETACH:
        if (pCore)
            pCore->Close();
        break;
    }
    return TRUE;
}

DWORD WINAPI MulNX_CS2_Start(void*) {
    try {
        // 创建核心
        pCore = MulNX::Core::Core::Create("CS2OBTool");
        // 将DLL模块句柄传递给核心，以便后续使用
        pCore->hMyOriginModule = hOriginModule;

        // 注册所有模块
        (*pCore->ModuleManager())
            .CreateModule<HookManager>("HookManager")
            .CreateSystemModules()// 创建所有系统模块，这是框架运行的基础
            .CreateModule<DLLLoadDispatcher>("DLLLoadDispatcher")
            .CreateModule<FileRedirector>("FileRedirector")
            .CreateModule<MulNX::ShaderCompiler>("ShaderCompiler")
            .CreateModule<MulNX::GraphicsManager>("GraphicsManager")
            .CreateModule<WebSocketManager>("WebSocketManager")
            .CreateModule<TimeLiner>("TimeLiner")
            // 底层CS2支持
            .CreateModule<CSController>("CSController")
            // CS2关键接口
            .CreateModule<HookConsole>("HookConsole")
            .CreateModule<HookEntitySystem>("HookEntitySystem")
            .CreateModule<TimeController>("TimeController")
            .CreateModule<ViewController>("ViewController")
            // CS2功能模块
            .CreateModule<AdvancedViewController>("AdvancedViewController")
            .CreateModule<FreeCameraController>("FreeCameraController")
            .CreateModule<ObserverController>("ObserverController")
            .CreateModule<PlayerFlashController>("PlayerFlashController")
            .CreateModule<DeathMsgController>("DeathMsgController")
            .CreateModule<ESPController>("ESPController")
            .CreateModule<SkinController>("SkinController")
            .CreateModule<TeamIDController>("TeamIDController")
            .CreateModule<EntityListScanner>("EntityListScanner")
            .CreateModule<POVFixer>("POVFixer")
            // 玩家强相关
            .CreateModule<PlayerHub>("PlayerHub")
            .CreateModule<ProjectileTracker>("ProjectileTracker")
            .CreateModule<NameController>("NameController")
            .CreateModule<GlowController>("GlowController")
            .CreateModule<SmokeController>("SmokeController")
            .CreateModule<SpeakingController>("SpeakingController")
            .CreateModule<KeyboardOverlay>("KeyboardOverlay")
            .CreateModule<PlayerSpotRenderController>("PlayerSpotRenderController")
            .CreateModule<PlayerSpotColorController>("PlayerSpotColorController")
            .CreateModule<BombSpotController>("BombSpotController")
            // 摄像机系统
            .CreateModule<CameraSystem>("CameraSystem")
            .CreateModule<WorkspaceManager>("WorkspaceManager")
            .CreateModule<ProjectManager>("ProjectManager")
            .CreateModule<SolutionManager>("SolutionManager")
            .CreateModule<ElementManager>("ElementManager")
            // Demos
            .CreateModule<DemoSystem>("DemoSystem")
            .CreateModule<DemoAnalyzer>("DemoAnalyzer")
            .CreateModule<DemoHelper>("DemoHelper")
            .CreateModule<DemoJSONReader>("DemoJSONReader")
            .CreateModule<RecordTaskConfiger>("RecordTaskConfiger")
            .CreateModule<RecordTaskMaker>("RecordTaskMaker")
            .CreateModule<DemoRecorder>("DemoRecorder")
            // 较为上层
            .CreateModule<MiniMap>("MiniMap")
            .CreateModule<VirtualUser>("VirtualUser")
            .CreateModule<GameCfgManager>("GameCfgManager")
            .CreateModule<GameSettingsManager>("GameSettingsManager")
            // 音视频
            .CreateModule<MediaSystem>("MediaSystem")
            .CreateModule<MediaParamManager>("MediaParamManager")
            .CreateModule<VCD3D11Manager>("VCD3D11Manager")
            .CreateModule<AudioCapturer>("AudioCapturer")
            .CreateModule<VideoCapturer>("VideoCapturer")
            .CreateModule<AEncodeHelper>("AEncodeHelper")
            .CreateModule<VEncodeHelper>("VEncodeHelper")
            .CreateModule<MediaRecorder>("MediaRecorder")
            .CreateModule<MediaProcesser>("MediaProcesser")
            // 管理
            .CreateModule<MulNXController>("MulNXController")
            .CreateModule<UIDocker>("UIDocker")
            ;

        // 启动核心
        pCore->EntryInit(pCore);

        auto pHookManager = pCore->ModuleManager()->FindModule<HookManager>("HookManager");

        if (MulNXInfo::IsDebugVersion) {
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/Play"_hash);
            rp->str1 = "111";
            pHookManager->PublishAsync(std::move(msg));
            std::thread([]() {
                MessageBoxW(NULL, L"MulNX 注入成功！", L"MulNX", MB_OK | MB_ICONINFORMATION);
                }).detach();
        }
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