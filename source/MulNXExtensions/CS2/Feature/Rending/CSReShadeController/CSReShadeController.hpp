#pragma once
#include <Intro/CSModuleBase.hpp>
#include <MulNXExtensions/MulNXReShadeBridge/IMulNXReShadeBridge.hpp>

class CSReShadeController final :public CSModuleBase {
    IMulNXReShadeBridge* pMulNXReShadeBridge = nullptr;

    using OMSetRenderTargets_t = void(STDMETHODCALLTYPE*)(
        ID3D11DeviceContext* This,
        UINT NumViews,
        ID3D11RenderTargetView* const* ppRenderTargetViews,
        ID3D11DepthStencilView* pDepthStencilView);
    
    std::unique_ptr<MulNX::Hook> hkOMSetRenderTargets = nullptr;

    bool Init()override;
};