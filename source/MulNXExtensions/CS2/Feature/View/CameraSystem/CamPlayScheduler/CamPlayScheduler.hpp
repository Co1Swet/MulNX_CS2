#pragma once
#include <CameraSystem/CameraSystem.hpp>
#include <CameraSystem/FreeCameraPath/FreeCameraPath.hpp>
#include <CameraSystem/CamDrawer/CamDrawer.hpp>

class CamPlayScheduler final :public CamSysModule {
    class PlaySlot final {
    public:
        std::shared_ptr<const FreeCameraPath> pCampath = nullptr;
        double offsetTime = 0.0f;
        bool isActiveMode = false;
    };
    std::vector<PlaySlot>playslots{};
    CampathManager* pCampathManager = nullptr;
    CamDrawer* pCamDrawer = nullptr;

    std::atomic<bool> drawCam = false;
    std::atomic<bool> camOverride = true;

    std::atomic<bool> needDrawCamera = false;
    MulNX::NewestBuffer<MulNX::Math::View> drawCamera;

    void DrawPlayCamera();
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg);
public:
    void Menu();
    bool HandleUpdate(CameraSystemIO* IO);
};