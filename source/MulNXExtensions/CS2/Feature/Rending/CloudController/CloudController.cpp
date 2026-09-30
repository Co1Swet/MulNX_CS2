#include "CloudController.hpp"

void CloudController::Menu() {
    MulNX::UI::Checkbox("隐藏云层", this->hideCloud);
    MulNX::UI::Checkbox("隐藏太阳", this->hideSun);
}

bool CloudController::Init() {
    this->pSceneSystem = this->FindModule<SceneSystem>("SceneSystem");
    this->pGraphicsManager = this->FindModule<MulNX::GraphicsManager>("GraphicsManager");

    
    this->SubscribeSync("GraphicsSync/ID3D11Device/Ready", [this](MulNX::Message& msg) {
        auto&& [pDevice] = msg.Access<ID3D11Device*>();

        // 创建 no-draw BlendState
        D3D11_BLEND_DESC blendDesc = {};
        blendDesc.AlphaToCoverageEnable = FALSE;
        blendDesc.IndependentBlendEnable = FALSE;
        blendDesc.RenderTarget[0].BlendEnable = FALSE;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = 0; // 不写任何通道
        pDevice->CreateBlendState(&blendDesc, &this->pNoDrawBlend);

        // 创建 no-draw DepthStencilState
        D3D11_DEPTH_STENCIL_DESC dsDesc = {};
        dsDesc.DepthEnable = FALSE;
        dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        dsDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
        dsDesc.StencilEnable = FALSE;
        pDevice->CreateDepthStencilState(&dsDesc, &this->pNoDrawDepth);
        });

    this->SubscribeSync("GraphicsSync/ID3D11DeviceContext/Ready", [this](MulNX::Message& msg) {
        auto&& [pContext] = msg.Access<ID3D11DeviceContext*>();

        this->hkOMSetBlendState = MulNX::Hook::Create((uint8_t*)IVClass::Assume(pContext)->GetVFuncPtr(35), [this](MulNX::Hook* hk, RegContext* ctx) {
            auto* pThis = (ID3D11DeviceContext*)ctx->rcx;
            ID3D11BlendState* pBlendState = (ID3D11BlendState*)ctx->rdx;
            const FLOAT* pBlendFactor = (const FLOAT*)ctx->r8;
            UINT SampleMask = *(UINT*)&ctx->r9;

            if (!this->bBlockColor)return MulNX::Hook::Then::Continue;

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

            ctx->rdx = (uint64_t)this->pNoDrawBlend.Get();
            ctx->r8 = 0;

            return MulNX::Hook::Then::Continue;
            }).value();
        this->RegisterAttachHook(this->hkOMSetBlendState, "OMSetBlendState");

        this->hkOMSetDepthStencilState = MulNX::Hook::Create((uint8_t*)IVClass::Assume(pContext)->GetVFuncPtr(36), [this](MulNX::Hook* hk, RegContext* ctx) {
            auto* pThis = (ID3D11DeviceContext*)ctx->rcx;
            ID3D11DepthStencilState* pDepthStencilState = (ID3D11DepthStencilState*)ctx->rdx;
            UINT StencilRef = *(UINT*)&ctx->r8;

            if (!this->bBlockDepth)return MulNX::Hook::Then::Continue;

            // 保存游戏“想设置的状态”
            this->pSavedDepthStencilState = pDepthStencilState;
            this->savedStencilRef = StencilRef;

            // 替换成 no-draw
            ctx->rdx = (uint64_t)this->pNoDrawDepth.Get();
            ctx->r8 = 0;

            return MulNX::Hook::Then::Return;
            }).value();
        this->RegisterAttachHook(this->hkOMSetDepthStencilState, "OMSetDepthStencilState");
        });

    this->SubscribeSync("Hook/LoadLibraryExW/scenesystem.dll", [this](MulNX::Message& msg) {
        this->pDrawSceneData = this->pSceneSystem->pDrawSceneData;
        this->pDrawCurrentPrimitives = this->pSceneSystem->pDrawCurrentPrimitives;

        this->hkDrawSceneData = MulNX::Hook::Create((uint8_t*)this->pDrawSceneData, [this](MulNX::Hook* hk, RegContext* ctx) {
            auto* pDrawingData = (CS2::DrawingData*)ctx->rcx;
            auto* pSceneData = (CS2::CBaseSceneData*)ctx->rdx;

            auto OnExit = [&]()->MulNX::Hook::Then {
                auto pRawFunc = (SceneSystem::DrawSceneData_t)hk->pMaybeRawFunc;
                pRawFunc(pDrawingData, pSceneData);
                this->pDrawCurrentPrimitives(pDrawingData);
                return MulNX::Hook::Then::Return;
                };

            if (!pSceneData)return OnExit();
            if (!pSceneData->material)return OnExit();
            const char* matName = pSceneData->material->GetName();
            if (!this->NeedHide(matName))return OnExit();

            pDrawingData->pSoftwareCommandList->QueueCallback(&this->wrapBlock);

            auto pRawFunc = (SceneSystem::DrawSceneData_t)hk->pMaybeRawFunc;
            pRawFunc(pDrawingData, pSceneData);
            this->pDrawCurrentPrimitives(pDrawingData);

            pDrawingData->pSoftwareCommandList->QueueCallback(&this->wrapUnblock);

            return MulNX::Hook::Then::Return;
            }).value();
        this->RegisterAttachHook(this->hkDrawSceneData, "DrawSceneData");
        });

    this->UIRegisterCallback("UI.3DVision", [this](auto&&...) { this->Menu(); });

    return true;
}

bool CloudController::NeedHide(const char* matName) {
    if (strstr(matName, "clouds") && this->hideCloud.load(std::memory_order_acquire)) {
        return true;
    }
    else if (strstr(matName, "sun_disc_glow") && this->hideSun.load(std::memory_order_acquire)) {
        return true;
    }
    return false;
}

void CloudController::DoBlock() {
    if (this->bBlockColor)return;
    auto* pContext = this->pGraphicsManager->pd3dContext;

    pContext->OMGetBlendState(
        this->pSavedBlendState.ReleaseAndGetAddressOf(),
        this->savedBlendFactor,
        &this->savedSampleMask
    );
    this->bBlockColor = true;

    auto pRawOMSetBlendState = (void(__fastcall*)(
        ID3D11DeviceContext*, ID3D11BlendState*, const FLOAT*, UINT)
        ) this->hkOMSetBlendState->pMaybeRawFunc;

    pRawOMSetBlendState(pContext, this->pNoDrawBlend.Get(), nullptr, this->savedSampleMask);

    pContext->OMGetDepthStencilState(
        this->pSavedDepthStencilState.ReleaseAndGetAddressOf(),
        &this->savedStencilRef
    );
    this->bBlockDepth = true;

    auto pRawOMSetDepthStencilState = (void(__fastcall*)(
        ID3D11DeviceContext*, ID3D11DepthStencilState*, UINT)
        ) this->hkOMSetDepthStencilState->pMaybeRawFunc;

    pRawOMSetDepthStencilState(pContext, this->pNoDrawDepth.Get(), 0);
}
void CloudController::DoUnblock() {
    if (!this->bBlockColor)return;
    auto* pContext = this->pGraphicsManager->pd3dContext;

    this->bBlockColor = false;
    this->bBlockDepth = false;

    auto pRawOMSetBlendState = (void(__fastcall*)(
        ID3D11DeviceContext*, ID3D11BlendState*, const FLOAT*, UINT)
        ) this->hkOMSetBlendState->pMaybeRawFunc;

    pRawOMSetBlendState(
        pContext,
        this->pSavedBlendState.Get(),
        this->savedBlendFactor,
        this->savedSampleMask
    );
    this->pSavedBlendState.Reset();

    auto pRawOMSetDepthStencilState = (void(__fastcall*)(
        ID3D11DeviceContext*, ID3D11DepthStencilState*, UINT)
        ) this->hkOMSetDepthStencilState->pMaybeRawFunc;

    pRawOMSetDepthStencilState(
        pContext,
        this->pSavedDepthStencilState.Get(),
        this->savedStencilRef
    );
    this->pSavedDepthStencilState.Reset();
}