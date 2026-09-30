#include "RendCtrlAPI.hpp"

bool RendCtrlAPI::Init() {
    this->pGraphicsManager = this->FindModule<MulNX::GraphicsManager>("GraphicsManager");

    this->SubscribeSync("GraphicsSync/ID3D11Device/Ready", [this](MulNX::Message& msg) {
        auto&& [pDevice] = msg.Access<ID3D11Device*>();

        // 创建 no-draw BlendState
        D3D11_BLEND_DESC blendDesc = {};
        blendDesc.AlphaToCoverageEnable = FALSE;
        blendDesc.IndependentBlendEnable = FALSE;
        blendDesc.RenderTarget[0].BlendEnable = FALSE;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = 0; // 不写任何通道
        pDevice->CreateBlendState(&blendDesc, &this->RT_pBSNoDraw);

        // 创建 no-draw DepthStencilState
        D3D11_DEPTH_STENCIL_DESC dsDesc = {};
        dsDesc.DepthEnable = FALSE;
        dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        dsDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
        dsDesc.StencilEnable = FALSE;
        pDevice->CreateDepthStencilState(&dsDesc, &this->RT_pDSSNoDrawDepth);
        });

    this->SubscribeSync("GraphicsSync/ID3D11DeviceContext/Ready", [this](MulNX::Message& msg) {
        auto&& [pContext] = msg.Access<ID3D11DeviceContext*>();

        this->hkOMSetBlendState = MulNX::Hook::Create((uint8_t*)IVClass::Assume(pContext)->GetVFuncPtr(35),
            [this](MulNX::Hook* hk, RegContext* ctx) {
                return this->RT_OnOMSetBlendState(hk, ctx);
            }).value();
        this->RegisterAttachHook(this->hkOMSetBlendState, "OMSetBlendState");
        this->pRawOMSetBlendState = (OMSetBlendState_t)this->hkOMSetBlendState->pMaybeRawFunc;

        this->hkOMSetDepthStencilState = MulNX::Hook::Create((uint8_t*)IVClass::Assume(pContext)->GetVFuncPtr(36),
            [this](MulNX::Hook* hk, RegContext* ctx) {
                return this->RT_OnOMSetDepthStencilState(hk, ctx);
            }).value();
        this->RegisterAttachHook(this->hkOMSetDepthStencilState, "OMSetDepthStencilState");
        this->pRawOMSetDepthStencilState = (OMSetDepthStencilState_t)this->hkOMSetDepthStencilState->pMaybeRawFunc;
        });

    return true;
}

MulNX::Hook::Then RendCtrlAPI::RT_OnOMSetBlendState(MulNX::Hook* hk, RegContext* ctx) {
    auto* pThis = (ID3D11DeviceContext*)ctx->rcx;
    ID3D11BlendState* pBlendState = (ID3D11BlendState*)ctx->rdx;
    const FLOAT* pBlendFactor = (const FLOAT*)ctx->r8;
    UINT SampleMask = *(UINT*)&ctx->r9;

    if (!this->RT_bBlockColor)return MulNX::Hook::Then::Continue;

    this->pSavedBlendState = pBlendState;

    if (pBlendFactor) {
        memcpy(this->savedBlendFactor, pBlendFactor, sizeof(this->savedBlendFactor));
    }
    else {
        this->savedBlendFactor[0] =
            this->savedBlendFactor[1] =
            this->savedBlendFactor[2] =
            this->savedBlendFactor[3] = 1.0f;
    }
    this->savedSampleMask = SampleMask;

    ctx->rdx = (uint64_t)this->RT_pBSNoDraw.Get();
    ctx->r8 = 0;

    return MulNX::Hook::Then::Continue;
}
void RendCtrlAPI::RT_BlockColor() {
    if (this->RT_bBlockColor)return;
    this->RT_bBlockColor = true;

    auto* pContext = this->pGraphicsManager->pd3dContext;
    pContext->OMGetBlendState(
        this->pSavedBlendState.ReleaseAndGetAddressOf(),
        this->savedBlendFactor,
        &this->savedSampleMask
    );
    
    this->pRawOMSetBlendState(pContext, this->RT_pBSNoDraw.Get(), nullptr, this->savedSampleMask);
}
void RendCtrlAPI::RT_UnblockColor() {
    if (!this->RT_bBlockColor)return;
    this->RT_bBlockColor = false;
    auto* pContext = this->pGraphicsManager->pd3dContext;

    this->pRawOMSetBlendState(
        pContext,
        this->pSavedBlendState.Get(),
        this->savedBlendFactor,
        this->savedSampleMask
    );
    this->pSavedBlendState.Reset();
}
// -----------------------------------------------------------------------------------------------------
MulNX::Hook::Then RendCtrlAPI::RT_OnOMSetDepthStencilState(MulNX::Hook* hk, RegContext* ctx) {
    auto* pThis = (ID3D11DeviceContext*)ctx->rcx;
    ID3D11DepthStencilState* pDepthStencilState = (ID3D11DepthStencilState*)ctx->rdx;
    UINT StencilRef = *(UINT*)&ctx->r8;

    if (!this->RT_bBlockDepth)return MulNX::Hook::Then::Continue;

    // 保存游戏“想设置的状态”
    this->pSavedDepthStencilState = pDepthStencilState;
    this->savedStencilRef = StencilRef;

    return MulNX::Hook::Then::Return;
}
void RendCtrlAPI::RT_BlockDepth() {
    if (this->RT_bBlockDepth)return;
    this->RT_bBlockDepth = true;

    auto* pContext = this->pGraphicsManager->pd3dContext;
    pContext->OMGetDepthStencilState(
        this->pSavedDepthStencilState.ReleaseAndGetAddressOf(),
        &this->savedStencilRef
    );

    this->pRawOMSetDepthStencilState(pContext, this->RT_pDSSNoDrawDepth.Get(), 0);
}
void RendCtrlAPI::RT_UnblockDepth() {
    if (!this->RT_bBlockDepth)return;
    this->RT_bBlockDepth = false;
    auto* pContext = this->pGraphicsManager->pd3dContext;

    this->pRawOMSetDepthStencilState(
        pContext,
        this->pSavedDepthStencilState.Get(),
        this->savedStencilRef
    );
    this->pSavedDepthStencilState.Reset();
}