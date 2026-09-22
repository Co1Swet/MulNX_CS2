#pragma once
#include <CameraSystem/CameraSystem.hpp>
#include <CameraSystem/CameraSystemIO/CameraSystemIO.hpp>
#include <CameraSystem/FreeCameraPath/FreeCameraPath.hpp>
#include <CameraSystem/CameraDrawer/CameraDrawer.hpp>
#include "CamPlayRequest.hpp"

class CamPlayScheduler final :public CamSysModule {
    class PlaySlot final {
    public:
        std::shared_ptr<const FreeCameraPath> pCampath = nullptr;
        float offsetTime = 0.0f;
        bool isActiveMode = false;
    };
    std::vector<PlaySlot>playslots{};
    CampathManager* pCampathManager = nullptr;
    CameraDrawer* pCamDrawer = nullptr;

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