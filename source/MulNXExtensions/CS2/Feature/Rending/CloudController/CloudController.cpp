#include "CloudController.hpp"

void CloudController::Menu() {
    MulNX::UI::Checkbox("隐藏云层", this->hideCloud);
    MulNX::UI::Checkbox("隐藏太阳", this->hideSun);
}

bool CloudController::Init() {
    this->pSceneSystem = this->FindModule<SceneSystem>("SceneSystem");

    this->SubscribeSync("Hook/LoadLibraryExW/scenesystem.dll", [this](MulNX::Message& msg) {
        this->pDrawSceneData = this->pSceneSystem->pDrawSceneData;
        this->pDrawCurrentPrimitives = this->pSceneSystem->pDrawCurrentPrimitives;

        this->hkDrawSceneData = MulNX::Hook::Create((uint8_t*)this->pDrawSceneData, [this](MulNX::Hook* hk, RegContext* ctx) {
            auto* pDrawingData = (CS2::DrawingData*)ctx->rcx;
            auto* pSceneData = (CS2::CBaseSceneData*)ctx->rdx;

            if (!pSceneData->material)return MulNX::Hook::Then::Continue;
            const char* matName = pSceneData->material->GetName();
            if (!matName)return MulNX::Hook::Then::Continue;
            if (!this->NeedHide(matName))return MulNX::Hook::Then::Continue;

            pDrawingData->pSoftwareCommandList->QueueCallback(new WrapCS2RenderCallback([this]() {
                this->bBlockColor = true;
                this->bBlockDepth = true;
                }));

            auto pRawFunc = (SceneSystem::DrawSceneData_t)hk->pMaybeRawFunc;
            pRawFunc(pDrawingData, pSceneData);
            this->pDrawCurrentPrimitives(pDrawingData);

            pDrawingData->pSoftwareCommandList->QueueCallback(new WrapCS2RenderCallback([this]() {
                this->bBlockColor = false;
                this->bBlockDepth = false;
                }));

            return MulNX::Hook::Then::Return;
            }).value();
        this->RegisterAttachHook(this->hkDrawSceneData, "DrawSceneData");
        });

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
            const FLOAT* BlendFactor = (const FLOAT*)ctx->r8;
            UINT SampleMask = *(UINT*)&ctx->r9;

            if (this->bBlockColor && this->pNoDrawBlend) {
                ctx->rdx = (uint64_t)this->pNoDrawBlend.Get();
                ctx->r8 = 0; // BlendFactor = nullptr
            }

            return MulNX::Hook::Then::Continue;
            }).value();
        this->RegisterAttachHook(this->hkOMSetBlendState, "OMSetBlendState");

        this->hkOMSetDepthStencilState = MulNX::Hook::Create((uint8_t*)IVClass::Assume(pContext)->GetVFuncPtr(36), [this](MulNX::Hook* hk, RegContext* ctx) {
            auto* pThis = (ID3D11DeviceContext*)ctx->rcx;
            ID3D11DepthStencilState* pDepthStencilState = (ID3D11DepthStencilState*)ctx->rdx;
            UINT StencilRef = *(UINT*)&ctx->r8;

            if (this->bBlockDepth && this->pNoDrawDepth) {
                ctx->rdx = (uint64_t)this->pNoDrawDepth.Get();
                ctx->r8 = 0; // StencilRef = 0
            }

            return MulNX::Hook::Then::Continue;
            }).value();
        this->RegisterAttachHook(this->hkOMSetDepthStencilState, "OMSetDepthStencilState");
        });

    this->UIRegisterCallback("UI.3DVision", [this](auto&&...) {this->Menu();});

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