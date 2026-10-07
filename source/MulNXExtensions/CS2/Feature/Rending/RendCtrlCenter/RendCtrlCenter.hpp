#pragma once
#include <Mirror/SceneSystem/SceneSystem.hpp>

class RendCtrlCenter final :public CSModuleBase {
    SceneSystem* pSceneSystem = nullptr;
    class RendCtrlAPI* pRendCtrlAPI = nullptr;
    class CloudController* pCloudController = nullptr;

    std::unique_ptr<MulNX::Hook> hkDrawSceneData = nullptr;

    SceneSystem::DrawCurrentPrimitives_t pDrawCurrentPrimitives = nullptr;
    SceneSystem::DrawSceneData_t pDrawSceneData = nullptr;

    std::atomic<CS2::SoftwareCommandList*> pLastListBeforePanorama = nullptr;
    std::unique_ptr<MulNX::Hook> hkInitDrawingData = nullptr;
    SceneSystem::InitDrawingData_t pRawInitDrawingData = nullptr;
    MulNX::Hook::Then OnInitDrawingData(MulNX::Hook* hk, RegContext* ctx);

    std::unique_ptr<MulNX::Hook> hkCommit = nullptr;
    MulNX::Hook::Then OnCommit(MulNX::Hook* hk, RegContext* ctx);

    bool Init()override;
};