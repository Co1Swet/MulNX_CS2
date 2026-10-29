#pragma once
#include <CameraSystem/CameraSystem.hpp>
#include <CameraSystem/CameraSystemIO/CameraSystemIO.hpp>
#include <CameraSystem/FreeCameraPath/FreeCameraPath.hpp>
#include <CameraSystem/CameraDrawer/CameraDrawer.hpp>
#include "CamPlayRequest.hpp"
#include <MulNXExtensions/WebSocketManager/WebSocketMixin.hpp>

class CamPlayScheduler final :public CamSysModule, public WebSocketMixin<CamPlayScheduler> {
    class PlaySlot final {
    public:
        std::shared_ptr<const FreeCameraPath> pCampath = nullptr;
        float offsetTime = 0.0f;
        bool isActiveMode = false;
        bool tracked = false;
    };
    std::vector<PlaySlot>playslots{};
    CampathManager* pCampathManager = nullptr;
    CameraDrawer* pCamDrawer = nullptr;
    class CamMacroManager* pCamMacroManager = nullptr;
    class CamPackManager* pCamPackManager = nullptr;
    uint64_t currentGeneration = 0;
    uint64_t observedPackRevision = 0;
    bool tracked = false;
    bool started = false;
    uint64_t requestId = 0;
    uint64_t generation = 0;
    std::string expectedPack;
    std::string macroName;

    void TrackedState(const char* state, const std::string& reason = {});
    void ClearTracked(const char* state, const std::string& reason);
    bool ResolveMacro(const std::string& name, std::vector<PlaySlot>& slots, std::string& reason);


    std::atomic<bool> drawCam = false;
    std::atomic<bool> camOverride = true;

    std::atomic<bool> needDrawCamera = false;
    MulNX::NewestBuffer<MulNX::Math::Frame> drawCamera;

    void DrawPlayCamera();
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg);
public:
    void Menu();
    bool HandleUpdate(CameraSystemIO* IO);
};