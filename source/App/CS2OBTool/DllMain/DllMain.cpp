#include "DllMain.hpp"
#include <App/CS2OBTool/UIDocker/UIDocker.hpp>
#include <MulNXExtensions/WinBaseHooks/WinBaseHooks.hpp>
#include <MulNXExtensions/CS2/CS2s.hpp>
#include <MulNXExtensions/MulNXController/MulNXController.hpp>
#include <MulNXExtensions/WebSocketManager/WebSocketManager.hpp>
#include <MulNXExtensions/MediaSystem/Medias.hpp>
#include <MulNXExtensions/TimeLiner/TimeLiner.hpp>
#include <MulNXExtensions/TimeLiner/FlowClock/FlowClock.hpp>

static HANDLE hInitCompleteEvent = nullptr;
BOOL APIENTRY DllMain(HMODULE, DWORD, LPVOID) { return TRUE; }
void StartImpl(HMODULE& hModule) {
    hModule = GetModuleHandleW(L"CS2OBTool.dll");
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
        .CreateModule<FlowClock>("FlowClock")
        // 以上为通用逻辑

        // 底层CS2支持
        .CreateModule<CSController>("CSController")

        // CS2 关键底层支持
        .CreateModule<GFuncBind>("GFuncBind")
        .CreateModule<ClientEntitySystem>("ClientEntitySystem")
        .CreateModule<BackgroundEntityScan>("BackgroundEntityScan")
        .CreateModule<HookConsole>("HookConsole")
        .CreateModule<HookView>("HookView")
        .CreateModule<HookGSI>("HookGSI")
        .CreateModule<CS2Hash>("CS2Hash")
        .CreateModule<TimeController>("TimeController")
        .CreateModule<PlayerHub>("PlayerHub")
        .CreateModule<DemoPlaying>("DemoPlaying") // 承担了时间适配器责任
        .CreateModule<NameController>("NameController")

        // CS2 基础支持
        .CreateModule<ConsoleOutput>("ConsoleOutput")
        .CreateModule<HookEntitySystem>("HookEntitySystem")
        .CreateModule<HookGameEvents>("HookGameEvents")
        .CreateModule<ParticleManager>("ParticleManager")
        .CreateModule<MaterialSystem>("MaterialSystem")
        .CreateModule<SceneSystem>("SceneSystem")
        .CreateModule<ResourceSystem>("ResourceSystem")
        .CreateModule<ParticleSystemMgr>("ParticleSystemMgr")

        // CS2 摄像机系统
        .CreateModule<CameraSystem>("CameraSystem")
        .CreateModule<WorkspaceManager>("WorkspaceManager")
        .CreateModule<ProjectManager>("ProjectManager")
        .CreateModule<SolutionManager>("SolutionManager")
        .CreateModule<ElementManager>("ElementManager")

        // CS2 视角控制功能模块
        .CreateModule<FreeCameraController>("FreeCameraController")
        .CreateModule<AdvancedViewController>("AdvancedViewController")
        .CreateModule<ObserverController>("ObserverController")
        .CreateModule<ProjectileTracker>("ProjectileTracker")
        .CreateModule<ViewSmoother>("ViewSmoother")

        // CS2 视觉叠加功能模块
        .CreateModule<ESPBox>("ESPBox")
        .CreateModule<ESPSkeleton>("ESPSkeleton")
        .CreateModule<SpecTargetUI>("SpecTargetUI")

        // CS2 视觉功能模块
        .CreateModule<GlowController>("GlowController")
        .CreateModule<PlayerFlashController>("PlayerFlashController")
        .CreateModule<DeathMsgController>("DeathMsgController")
        .CreateModule<SkyController>("SkyController")
        .CreateModule<SkinController>("SkinController")
        .CreateModule<TrailsController>("TrailsController")
        .CreateModule<SmokeController>("SmokeController")
        .CreateModule<ViewModelController>("ViewModelController")
        .CreateModule<WorldDraw>("WorldDraw")

        // CS2 视觉（HUD）功能模块
        .CreateModule<TeamIDColorController>("TeamIDColorController")
        .CreateModule<TeamIDRenderController>("TeamIDRenderController")
        .CreateModule<PlayerSpotRenderController>("PlayerSpotRenderController")
        .CreateModule<PlayerSpotColorController>("PlayerSpotColorController")
        .CreateModule<BombSpotController>("BombSpotController")
        .CreateModule<HookTeamCounter>("HookTeamCounter")
        .CreateModule<FlashRenderController>("FlashRenderController")
        .CreateModule<HookHealthAmmoCenter>("HookHealthAmmoCenter")

        // CS2 声音功能模块
        .CreateModule<AntiVoiceBan>("AntiVoiceBan")
        .CreateModule<ReShowSpeaker>("ReShowSpeaker")
        .CreateModule<SpeakingController>("SpeakingController")
        .CreateModule<HitSoundFix>("HitSoundFix")
        .CreateModule<SoundCircleFix>("SoundCircleFix")

        // CS2 Demo 相关模块
        .CreateModule<DemState>("DemState")
        .CreateModule<HookDemo>("HookDemo")
        .CreateModule<DemoHelper>("DemoHelper")
        
        .CreateModule<DemoFiles>("DemoFiles")
        .CreateModule<DemoAnalyzer>("DemoAnalyzer")
        .CreateModule<DemoJSONReader>("DemoJSONReader")

        .CreateModule<RecordTaskConfiger>("RecordTaskConfiger")
        .CreateModule<RecordTaskMaker>("RecordTaskMaker")
        .CreateModule<DemoRecorder>("DemoRecorder")
        
        .CreateModule<DemoEventsRender>("DemoEventsRender")
        .CreateModule<DemoPlayerInfoRender>("DemoPlayerInfoRender")

        .CreateModule<DemoSystem>("DemoSystem")

        // CS2 外围功能
        .CreateModule<KeyboardOverlay>("KeyboardOverlay")
        .CreateModule<MiniMap>("MiniMap")
        .CreateModule<VirtualUser>("VirtualUser")
        .CreateModule<GameCfgManager>("GameCfgManager")
        .CreateModule<GameSettingsManager>("GameSettingsManager")
        .CreateModule<HSI>("HSI")
        .CreateModule<AutoCfgLoad>("AutoCfgLoad")

        // CS2 杂项功能
        .CreateModule<EntityListScanner>("EntityListScanner")
#ifdef _DEBUG
        .CreateModule<CS2Test>("CS2Test")
#endif
        // 音视频系统
        .CreateModule<MediaRunningState>("MediaRunningState")
        .CreateModule<MediaSystem>("MediaSystem")
        .CreateModule<MediaParamManager>("MediaParamManager")

        .CreateModule<AudioCapturer>("AudioCapturer")
        .CreateModule<AEncodeHelper>("AEncodeHelper")

        .CreateModule<VCD3D11Manager>("VCD3D11Manager")
        .CreateModule<BufferCopier>("BufferCopier")
        .CreateModule<TextureMapper>("TextureMapper")
        .CreateModule<VideoCapturer>("VideoCapturer")
        .CreateModule<VEncodeHelper>("VEncodeHelper")

        .CreateModule<MediaRecorder>("MediaRecorder")
        .CreateModule<MediaProcesser>("MediaProcesser")

        // CS2 录制合作
        .CreateModule<HookRecordCmd>("HookRecordCmd")
        .CreateModule<RecordFileRedirect>("RecordFileRedirect")
        .CreateModule<AdvancedRecord>("AdvancedRecord")

        // 管理
        .CreateModule<MulNXController>("MulNXController")
        .CreateModule<UIDocker>("UIDocker")
        ;
    
    // 启动核心
    core->EntryInit(core.get());
    SetEvent(hInitCompleteEvent);
    core->Driver()->WaitEnd();
}
DWORD WINAPI StartWrapper(LPVOID lpParam) {
    HMODULE hModule;
    try {
        StartImpl(hModule);
    }
    catch (std::exception& e) {
        SetEvent(hInitCompleteEvent);
        MulNX::ErrorTerminate("在启动时发生异常！异常描述：" + std::string(e.what()));
    }
    catch (...) {
        SetEvent(hInitCompleteEvent);
        MulNX::ErrorTerminate("在启动时发生未知异常！");
    }
    FreeLibraryAndExitThread(hModule, 0);
}
DWORD WINAPI MulNX_CS2_Start(void*) {
    hInitCompleteEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    HANDLE hInitThread = CreateThread(NULL, 0, StartWrapper, NULL, 0, NULL);
    WaitForSingleObject(hInitCompleteEvent, INFINITE);
    return 0;
}