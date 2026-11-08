#pragma once
#include <Mirror/SceneSystem/SceneSystem.hpp>

class RendCtrlCenter final :public CSModuleBase {
    SceneSystem* pSceneSystem = nullptr;
    class RendCtrlAPI* pRendCtrlAPI = nullptr;
    class CloudController* pCloudController = nullptr;
    class CSReShadeController* pCSReShadeController = nullptr;

    std::unique_ptr<MulNX::Hook> hkDrawSceneData = nullptr;
    MulNX::Hook::Then OnDrawSceneData(MulNX::Hook* hk, RegContext* ctx);

    SceneSystem::DrawCurrentPrimitives_t pDrawCurrentPrimitives = nullptr;
    SceneSystem::DrawSceneData_t pDrawSceneData = nullptr;

    inline static thread_local CS2::SoftwareCommandList* pLastListBeforePanorama = nullptr;
    std::unique_ptr<MulNX::Hook> hkInitDrawingData = nullptr;
    SceneSystem::InitDrawingData_t pRawInitDrawingData = nullptr;
    MulNX::Hook::Then OnInitDrawingData(MulNX::Hook* hk, RegContext* ctx);

    std::unique_ptr<MulNX::Hook> hkCommit = nullptr;
    MulNX::Hook::Then OnCommit(MulNX::Hook* hk, RegContext* ctx);

    std::unique_ptr<MulNX::Hook> hkPos_Ret_SceneSystem_CreateRenderContextPtr1_With_Fmt_LayerClear = nullptr;
    MulNX::Hook::Then OnCreateRenderContextPtr1_With_Fmt_LayerClear(MulNX::Hook* hk, RegContext* ctx);
    std::unique_ptr<MulNX::Hook> hkPos_Ret_SceneSystem_CreateRenderContextPtr2_With_UpdateBuffers = nullptr;
    MulNX::Hook::Then OnCreateRenderContextPtr2_With_UpdateBuffers(MulNX::Hook* hk, RegContext* ctx);

    bool Init()override;
};