#pragma once
#include <Mirror/SceneSystem/SceneSystem.hpp>
#include <MulNXExtensions/GraphicsManager/GraphicsManager.hpp>

class RendCtrlAPI final :public CSModuleBase {
    MulNX::GraphicsManager* pGraphicsManager = nullptr;
    // -------------------------------------------------------
    ComPtr<ID3D11BlendState> RT_pBSNoDraw = nullptr;
    std::unique_ptr<MulNX::Hook> hkOMSetBlendState = nullptr;
    using OMSetBlendState_t = void(*)(ID3D11DeviceContext*, ID3D11BlendState*, const FLOAT*, UINT);
    OMSetBlendState_t pRawOMSetBlendState = nullptr;

    bool RT_bBlockColor = false;
    ComPtr<ID3D11BlendState> pSavedBlendState = nullptr;
    FLOAT savedBlendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    UINT savedSampleMask = 0xffffffff;
    MulNX::Hook::Then RT_OnOMSetBlendState(MulNX::Hook* hk, RegContext* ctx);
    void RT_BlockColor();
    void RT_UnblockColor();
    // --------------------------------------------------------------------------------
    ComPtr<ID3D11DepthStencilState> RT_pDSSNoDrawDepth = nullptr;
    std::unique_ptr<MulNX::Hook> hkOMSetDepthStencilState = nullptr;
    using OMSetDepthStencilState_t = void(*)(ID3D11DeviceContext*, ID3D11DepthStencilState*, UINT);
    OMSetDepthStencilState_t pRawOMSetDepthStencilState;

    bool RT_bBlockDepth = false;
    ComPtr<ID3D11DepthStencilState> pSavedDepthStencilState = nullptr;
    UINT savedStencilRef = 0;
    MulNX::Hook::Then RT_OnOMSetDepthStencilState(MulNX::Hook* hk, RegContext* ctx);
    void RT_BlockDepth();
    void RT_UnblockDepth();

    bool Init()override;

    class ET_BlockColor final :public CS2::IRenderThreadCallback {
        RendCtrlAPI* pThis;
        void OnCallback(void)override { pThis->RT_BlockColor(); }
    public:
        ET_BlockColor(RendCtrlAPI* pThis) :pThis(pThis) {}
    };
    class ET_UnblockColor final :public CS2::IRenderThreadCallback {
        RendCtrlAPI* pThis;
        void OnCallback(void)override { pThis->RT_UnblockColor(); }
    public:
        ET_UnblockColor(RendCtrlAPI* pThis) :pThis(pThis) {}
    };
public:
    ET_BlockColor ET_blockColor{ this };
    ET_UnblockColor ET_unblockColor{ this };
private:
    class ET_BlockDepth final :public CS2::IRenderThreadCallback {
        RendCtrlAPI* pThis;
        void OnCallback(void)override { pThis->RT_BlockDepth(); }
    public:
        ET_BlockDepth(RendCtrlAPI* pThis) :pThis(pThis) {}
    };
    class ET_UnblockDepth final :public CS2::IRenderThreadCallback {
        RendCtrlAPI* pThis;
        void OnCallback(void)override { pThis->RT_UnblockDepth(); }
    public:
        ET_UnblockDepth(RendCtrlAPI* pThis) :pThis(pThis) {}
    };
public:
    ET_BlockDepth ET_blockDepth{ this };
    ET_UnblockDepth ET_unblockDepth{ this };
};