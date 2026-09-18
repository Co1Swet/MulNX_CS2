#pragma once
#include <CameraSystem/CameraSystem.hpp>
#include <CameraSystem/CameraSystemIO/CameraSystemIO.hpp>
#include <CameraSystem/FreeCameraPath/FreeCameraPath.hpp>
#include "CamPlayRequest.hpp"

class CamPlayScheduler final :public CamSysModule {
    class PlaySlot final {
    public:
        std::shared_ptr<const FreeCameraPath> pCampath = nullptr;
        float offestTime = 0.0f;
    };
    std::vector<PlaySlot>playslots{};
    ElementManager* pEManager = nullptr;

    bool Init()override;
    void ProcessMsg(MulNX::Message& msg);
public:
    bool HandleUpdate(CameraSystemIO* IO);
};