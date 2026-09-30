#pragma once
#include <Mirror/SceneSystem/SceneSystem.hpp>
#include <MulNXExtensions/GraphicsManager/GraphicsManager.hpp>
#include <d3d11.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class CloudController final :public CSModuleBase {
    MulNX::GraphicsManager* pGraphicsManager = nullptr;
    SceneSystem* pSceneSystem = nullptr;

    std::atomic<bool> hideCloud = false;
    std::atomic<bool> hideSun = false;

    ComPtr<ID3D11BlendState> pNoDrawBlend = nullptr;
    ComPtr<ID3D11DepthStencilState> pNoDrawDepth = nullptr;

    bool bBlockColor = false;
    bool bBlockDepth = false;

    ComPtr<ID3D11BlendState>        pSavedBlendState = nullptr;
    FLOAT                           savedBlendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    UINT                            savedSampleMask = 0xffffffff;

    ComPtr<ID3D11DepthStencilState> pSavedDepthStencilState = nullptr;
    UINT                            savedStencilRef = 0;

    std::unique_ptr<MulNX::Hook> hkDrawSceneData = nullptr;

    std::unique_ptr<MulNX::Hook> hkOMSetBlendState = nullptr;
    std::unique_ptr<MulNX::Hook> hkOMSetDepthStencilState = nullptr;

    SceneSystem::DrawCurrentPrimitives_t pDrawCurrentPrimitives = nullptr;
    SceneSystem::DrawSceneData_t pDrawSceneData = nullptr;

    void Menu();
    bool Init()override;
    bool NeedHide(const char* matName);

    void DoBlock();
    class WrapBlock final :public CS2::IRenderThreadCallback {
        void OnCallback(void)override { pThis->DoBlock(); }
    public:
        CloudController* pThis;
        WrapBlock(CloudController* pThis) :pThis(pThis) {}
    };
    WrapBlock wrapBlock{ this };

    void DoUnblock();
    class WrapUnblock final :public CS2::IRenderThreadCallback {
        void OnCallback(void)override { pThis->DoUnblock(); }
    public:
        CloudController* pThis;
        WrapUnblock(CloudController* pThis) :pThis(pThis) {}
    };
    WrapUnblock wrapUnblock{ this };
};