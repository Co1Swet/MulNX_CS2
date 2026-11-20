#pragma once
#include <CameraSystem/CameraSystem.hpp>
#include <CameraSystem/FreeCameraPath/FreeCameraPath.hpp>
#include <CameraSystem/CamDrawer/CamDrawer.hpp>

class CamPlayScheduler final :public CamSysModule<CamPlayScheduler> {
    class PlaySlot final {
    public:
        std::shared_ptr<const FreeCameraPath> pCampath = nullptr;
        double offsetTime = 0.0f;
        bool isActiveMode = false;
        // 时间未到起点时提前就位，相机停在首帧角度待命
        bool preRoll = false;
    };
    std::vector<PlaySlot>playslots{};
    CampathManager* pCampathManager = nullptr;
    CamDrawer* pCamDrawer = nullptr;

    MulReflBool(drawCam, false);
    MulReflBool(camOverride, true);

    std::atomic<bool> needDrawCamera = false;
    MulNX::NewestBuffer<MulNX::Math::View> drawCamera;

    void DrawPlayCamera();
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg);
public:
    void Menu();
    bool HandleUpdate(CameraSystemIO* IO);
};