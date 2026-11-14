#include "DllMain.hpp"
#include <App/CS2OBTool/UIDocker/UIDocker.hpp>
#include <MulNXExtensions/GraphicsManager/Graphicses.hpp>
#include <MulNXExtensions/WinBaseHooks/WinBaseHooks.hpp>
#include <MulNXExtensions/CS2/CS2s.hpp>
#include <MulNXExtensions/MediaSystem/Medias.hpp>
#include <MulNXExtensions/TimeLiner/TimeLiner.hpp>
#include <MulNXExtensions/TimeLiner/FlowClock/FlowClock.hpp>
#include <MulNXExtensions/TimeLiner/TimeMarker/TimeMarkerManager.hpp>

static HANDLE hInitCompleteEvent = nullptr;
static HANDLE hUnloadReadyEvent = nullptr;
BOOL APIENTRY DllMain(HMODULE, DWORD, LPVOID) { return TRUE; }
void StartImpl(HMODULE& hModule) {
    hModule = GetModuleHandleW(L"CS2OBTool.dll");
    auto core = MulNX::Core::Core::Create("CS2OBTool");
    core->hMyOriginModule = hModule;
    (*core->ModuleManager())
        .CreateSystemModules()
        .CreateModule<HookWindow>()
        .CreateModule<DLLLoadDispatcher>()
        .CreateModule<FileRedirector>()
        .CreateModule<TimeLiner>()
        .CreateModule<FlowClock>()
        .CreateModule<TimeMarkerManager>()

        // 图形组件
        .CreateModule<HookD3D11>()
        .CreateModule<MulNX::ShaderCompiler>()
        .CreateModule<MulNX::GraphicsManager>()
        .CreateModule<ChromaKeyOverlay>()

        // 底层CS2支持
        .CreateModule<CSController>()

        // CS2 关键底层支持
        .CreateModule<GFuncBind>()
        .CreateModule<HookDemo>()
        .CreateModule<ClientEntitySystem>()
        .CreateModule<BackgroundEntityScan>()
        .CreateModule<HookConsole>()
        .CreateModule<HookView>()
        .CreateModule<HookEntitySystem>()
        .CreateModule<HookGameEvents>()
        .CreateModule<HookGSI>()
        .CreateModule<HookMainLoop>()

        // CS2 子系统镜像
        .CreateModule<CS2Hash>()
        .CreateModule<ParticleManager>()
        .CreateModule<MaterialSystem>()
        .CreateModule<SceneSystem>()
        .CreateModule<ResourceSystem>()
        .CreateModule<ParticleSystemMgr>()
        .CreateModule<VFileSystem>()
        .CreateModule<CSDemoController>()
        .CreateModule<CCStrike15ItemSystem>()

        // CS2 自定义支持
        .CreateModule<ClientPanorama>()
        .CreateModule<DamageRecorder>()
        .CreateModule<ImGuiBinder>()
        .CreateModule<ConsoleOutput>()
        .CreateModule<TargetPicker>()
        .CreateModule<TimeController>()
        .CreateModule<NameController>()
        .CreateModule<PlayerHub>()
        .CreateModule<FileInjector>()

        // CS2 摄像机系统
        .CreateModule<CameraSystem>()
        .CreateModule<CamDrawer>()
        .CreateModule<CamPackManager>()
        .CreateModule<CampathManager>()
        .CreateModule<CamMacroManager>()
        .CreateModule<CamPlayScheduler>()

        // CS2 视角控制功能模块
        .CreateModule<FreeCameraController>()
        .CreateModule<AdvancedViewController>()
        .CreateModule<ObserverController>()
        .CreateModule<ProjectileTracker>()
        .CreateModule<AutoAim>()
        .CreateModule<AutoDepth>()
        .CreateModule<ViewSmoother>()

        // CS2 视觉叠加功能模块
        .CreateModule<ESPBox>()
        .CreateModule<ESPSkeleton>()
        .CreateModule<SpecTargetUI>()

        // CS2 视觉功能模块
        .CreateModule<GlowController>()
        .CreateModule<PlayerFlashController>()
        .CreateModule<DeathMsgController>()
        .CreateModule<SkyController>()
        .CreateModule<TrailsController>()
        .CreateModule<SmokeController>()
        //.CreateModule<WorldDraw>()

        // CS2 视觉（HUD）功能模块
        .CreateModule<TeamIDColorController>()
        .CreateModule<TeamIDRenderController>()
        .CreateModule<HookSpotVisible>()
        .CreateModule<HookSpotStyle>()
        .CreateModule<HookSpotColor>()
        .CreateModule<BombSpotController>()
        .CreateModule<HookTeamCounter>()
        .CreateModule<HookDamageReport>()
        .CreateModule<FlashRenderController>()
        .CreateModule<HookHealthAmmoCenter>()

        // CS2 模型相关模块
        .CreateModule<ViewModelController>()
        .CreateModule<SkinConfigger>()
        .CreateModule<SkinController>()
        .CreateModule<SkinModelFixer>()

        // CS2 渲染管线控制模块
        .CreateModule<CSReShadeController>()
        .CreateModule<RendCtrlAPI>()
        .CreateModule<RendCtrlCenter>()
        .CreateModule<CloudController>()

        // CS2 声音功能模块
        .CreateModule<AntiVoiceBan>()
        .CreateModule<ReShowSpeaker>()
        .CreateModule<SpeakingController>()
        .CreateModule<HitSoundFix>()
        .CreateModule<SoundCircleFix>()

        // CS2 Demo 相关模块
        .CreateModule<DemoSystem>()
        .CreateModule<DemoFiles>()
        .CreateModule<DemoAnalyzer>()
        .CreateModule<DemoJSONReader>()
        .CreateModule<DemEasyView>()
        .CreateModule<RoundRecordMaker>()
        .CreateModule<DemoRecordMaker>()
        .CreateModule<AutoRecordRule>()
        .CreateModule<PassRecordMaker>()
        .CreateModule<RecordTaskConfiger>()
        .CreateModule<RecordTaskMaker>()
        .CreateModule<DemRecordPassManager>()
        .CreateModule<DemClassifyPass>()
        .CreateModule<DemSortPass>()
        .CreateModule<DemMergePass>()
        .CreateModule<DemoRecorder>()
        .CreateModule<DemoEventsRender>()
        .CreateModule<DemoPlayerInfoRender>()

        // CS2 地图更换功能模块
        .CreateModule<MapState>()
        .CreateModule<MapProto>()
        .CreateModule<MapKeyValues>()
        .CreateModule<MapManifest>()
        .CreateModule<MapResource>()

        // CS2 外围功能
        .CreateModule<KeyboardOverlay>()
        .CreateModule<MiniMap>()
        .CreateModule<VirtualUser>()
        .CreateModule<GameCfgManager>()
        .CreateModule<GameSettingsManager>()
        .CreateModule<HSI>()
        .CreateModule<AutoCfgLoad>()

        // CS2 杂项功能
        .CreateModule<EntityListScanner>()
#ifdef _DEBUG
        .CreateModule<CS2Test>()
#endif
        // 音视频系统
        .CreateModule<MediaParamManager>()
        .CreateModule<OpenH264Encoder>()
        .CreateModule<H264Nvenc>()
        .CreateModule<AVCodecManager>()

        .CreateModule<MediaRunningState>()
        .CreateModule<MediaSystem>()

        .CreateModule<AudioCapturer>()
        .CreateModule<AEncodeHelper>()

        .CreateModule<VCD3D11Manager>()
        .CreateModule<BufferCopier>()
        .CreateModule<TextureMapper>()
        .CreateModule<VideoCapturer>()
        .CreateModule<VEncodeController>()

        .CreateModule<MediaRecorder>()
        .CreateModule<MediaProcesser>()

        // CS2 录制
        .CreateModule<HookRecordCmd>()
        .CreateModule<RecordFileRedirect>()
        .CreateModule<AdvancedRecord>()
        .CreateModule<AdTimeFix>()
        .CreateModule<PlayProtect>()

        .CreateModule<UIDocker>()
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
        if (hUnloadReadyEvent) {
            SetEvent(hUnloadReadyEvent);
            CloseHandle(hUnloadReadyEvent);
        }
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
    auto eventName = L"Local\\MulNX_CS2_UnloadReady_" + std::to_wstring(GetCurrentProcessId());
    hUnloadReadyEvent = CreateEventW(NULL, TRUE, FALSE, eventName.c_str());
    if (hUnloadReadyEvent) ResetEvent(hUnloadReadyEvent);
    hInitCompleteEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    HANDLE hInitThread = CreateThread(NULL, 0, StartWrapper, NULL, 0, NULL);
    WaitForSingleObject(hInitCompleteEvent, INFINITE);
    return 0;
}
