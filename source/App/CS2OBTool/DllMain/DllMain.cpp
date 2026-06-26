#include "DllMain.hpp"
#include <App/CS2OBTool/UIDocker/UIDocker.hpp>
#include <MulNXExtensions/DLLLoadDispatcher/DLLLoadDispatcher.hpp>
#include <MulNXExtensions/FileRedirector/FileRedirector.hpp>
#include <MulNXExtensions/CS2/CS2s.hpp>
#include <MulNXExtensions/MulNXController/MulNXController.hpp>
#include <MulNXExtensions/WebSocketManager/WebSocketManager.hpp>
#include <MulNXExtensions/MediaSystem/Media.hpp>
#include <MulNXExtensions/TimeLiner/TimeLiner.hpp>

static HANDLE hInitCompleteEvent = nullptr;
BOOL APIENTRY DllMain(HMODULE, DWORD, LPVOID) { return TRUE; }
void StartImpl(HMODULE& hModule) {
    hModule = GetModuleHandleW(L"CS2OBTool.dll");
    try {
        // 创建核心
        auto core = MulNX::Core::Core::Create("CS2OBTool");
        // 将DLL模块句柄传递给核心，以便后续使用
        core->hMyOriginModule = hModule;
        // 注册所有模块
        (*core->ModuleManager())
            .CreateSystemModules()// 创建所有系统模块，这是框架运行的基础
            .CreateModule<HookWindow>("HookWindow")
            .CreateModule<HookD3D11>("HookD3D11")
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
            .CreateModule<ConsoleOutput>("ConsoleOutput")
            .CreateModule<HookEntitySystem>("HookEntitySystem")
            .CreateModule<ParticleManager>("ParticleManager")
            .CreateModule<MaterialSystem>("MaterialSystem")
            .CreateModule<SceneSystem>("SceneSystem")
            .CreateModule<TimeController>("TimeController")
            .CreateModule<HookView>("HookView")
            .CreateModule<PlayerHub>("PlayerHub")
            // CS2功能模块
            .CreateModule<FreeCameraController>("FreeCameraController")
            .CreateModule<AdvancedViewController>("AdvancedViewController")
            .CreateModule<ObserverController>("ObserverController")
            .CreateModule<PlayerFlashController>("PlayerFlashController")
            .CreateModule<DeathMsgController>("DeathMsgController")
            .CreateModule<SkyController>("SkyController")
            .CreateModule<ESPController>("ESPController")
            .CreateModule<ESPSkeleton>("ESPSkeleton")
            .CreateModule<SkinController>("SkinController")
            .CreateModule<EntityListScanner>("EntityListScanner")
            // 玩家强相关
            .CreateModule<ProjectileTracker>("ProjectileTracker")
            .CreateModule<TrailsController>("TrailsController")
            .CreateModule<NameController>("NameController")
            .CreateModule<GlowController>("GlowController")
            .CreateModule<SmokeController>("SmokeController")
            .CreateModule<SpeakingController>("SpeakingController")
            .CreateModule<KeyboardOverlay>("KeyboardOverlay")
            .CreateModule<TeamIDController>("TeamIDController")
            .CreateModule<PlayerSpotRenderController>("PlayerSpotRenderController")
            .CreateModule<PlayerSpotColorController>("PlayerSpotColorController")
            .CreateModule<BombSpotController>("BombSpotController")
            .CreateModule<TeamCounterController>("TeamCounterController")
            // 摄像机系统
            .CreateModule<CameraSystem>("CameraSystem")
            .CreateModule<WorkspaceManager>("WorkspaceManager")
            .CreateModule<ProjectManager>("ProjectManager")
            .CreateModule<SolutionManager>("SolutionManager")
            .CreateModule<ElementManager>("ElementManager")
            // Demos
            .CreateModule<HookDemo>("HookDemo")
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
        core->EntryInit(core.get());
        if (MulNXInfo::IsDebugVersion) {
            auto pHookConsole = core->ModuleManager()->FindModule<HookConsole>("HookConsole");
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/Play"_hash);
            rp->str1 = "111";
            pHookConsole->PublishAsync(std::move(msg));
            std::thread([]() {
                MessageBoxW(NULL, L"MulNX 注入成功！", L"MulNX", MB_OK | MB_ICONINFORMATION);
                }).detach();
        }
        SetEvent(hInitCompleteEvent);
        core->Driver()->WaitEnd();
    }
    catch (std::exception& e) {
        MulNX::ErrorTerminate("在启动时发生异常！异常描述：" + std::string(e.what()));
    }
    catch (...) {
        MulNX::ErrorTerminate("在启动时发生未知异常！");
    }
}
DWORD WINAPI StartWrapper(LPVOID lpParam) {
    HMODULE hModule;
    StartImpl(hModule);
    FreeLibraryAndExitThread(hModule, 0);
}
DWORD WINAPI MulNX_CS2_Start(void*) {
    hInitCompleteEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    HANDLE hInitThread = CreateThread(NULL, 0, StartWrapper, NULL, 0, NULL);
    WaitForSingleObject(hInitCompleteEvent, INFINITE);
    return 0;
}