#pragma once
#include <Mirror/SceneSystem/SceneSystem.hpp>
#include <MulNXExtensions/MulNXReShadeBridge/IMulNXReShadeBridge.hpp>

class CSReShadeController final :public CSModuleBase {
    IMulNXReShadeBridge* pMulNXReShadeBridge = nullptr;

    enum class PanoramaRend {
        Idle,
        Pending,
        Fired
    };
    PanoramaRend RT_ePanoramaRend = PanoramaRend::Idle;

    using OMSetRenderTargets_t = void(STDMETHODCALLTYPE*)(
        ID3D11DeviceContext* This,
        UINT NumViews,
        ID3D11RenderTargetView* const* ppRenderTargetViews,
        ID3D11DepthStencilView* pDepthStencilView);
    
    std::unique_ptr<MulNX::Hook> hkOMSetRenderTargets = nullptr;

    void Menu()const;
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg);
    void BeforeRendPanorama(ID3D11DeviceContext* pD3D11Ctx);

    class AT2RT_SetPending final :public CS2::IRenderThreadCallback {
        CSReShadeController* pThis;
        void OnCallback(void)override { pThis->RT_ePanoramaRend = PanoramaRend::Pending; }
    public:
        AT2RT_SetPending(CSReShadeController* pThis) :pThis(pThis) {}
    };

    class AT2RT_SetIdle final :public CS2::IRenderThreadCallback {
        CSReShadeController* pThis;
        void OnCallback(void)override { pThis->RT_ePanoramaRend = PanoramaRend::Idle; }
    public:
        AT2RT_SetIdle(CSReShadeController* pThis) :pThis(pThis) {}
    };
public:
    inline bool IsConnnect()const { return this->pMulNXReShadeBridge != nullptr; }
    
    AT2RT_SetPending AT2RT_setPending{ this };
    AT2RT_SetIdle AT2RT_setIdle{ this };
};