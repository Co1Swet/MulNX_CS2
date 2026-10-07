#include "RendCtrlCenter.hpp"
#include <Feature/Rending/RendCtrlAPI/RendCtrlAPI.hpp>
#include <Feature/Rending/CloudController/CloudController.hpp>

bool RendCtrlCenter::Init() {
    this->pSceneSystem = this->FindModule<SceneSystem>("SceneSystem");
    this->pRendCtrlAPI = this->FindModule<RendCtrlAPI>("RendCtrlAPI");
    this->pCloudController = this->FindModule<CloudController>("CloudController");

    this->SubscribeSync("Hook/LoadLibraryExW/scenesystem.dll", [this](MulNX::Message& msg) {
        this->pDrawSceneData = this->pSceneSystem->pDrawSceneData;
        this->pDrawCurrentPrimitives = this->pSceneSystem->pDrawCurrentPrimitives;

        this->hkDrawSceneData = MulNX::Hook::Create((uint8_t*)this->pDrawSceneData, [this](MulNX::Hook* hk, RegContext* ctx) {
            auto* pDrawingData = (CS2::DrawingData*)ctx->rcx;
            auto* pSceneData = (CS2::CBaseSceneData*)ctx->rdx;

            bool hide = false;

            auto OnExit = [&]()->MulNX::Hook::Then {
                if (hide) {
                    pDrawingData->pSoftwareCommandList->QueueCallback(&this->pRendCtrlAPI->ET2RT_blockDepth);
                    pDrawingData->pSoftwareCommandList->QueueCallback(&this->pRendCtrlAPI->ET2RT_blockColor);
                }
                auto pRawFunc = (SceneSystem::DrawSceneData_t)hk->pMaybeRawFunc;
                pRawFunc(pDrawingData, pSceneData);
                this->pDrawCurrentPrimitives(pDrawingData);
                if (hide) {
                    pDrawingData->pSoftwareCommandList->QueueCallback(&this->pRendCtrlAPI->ET2RT_unblockDepth);
                    pDrawingData->pSoftwareCommandList->QueueCallback(&this->pRendCtrlAPI->ET2RT_unblockColor);
                }
                return MulNX::Hook::Then::Return;
                };

            if (!pSceneData)return OnExit();
            if (!pSceneData->material)return OnExit();
            const char* matName = pSceneData->material->GetName();

            hide = hide || this->pCloudController->NeedHide(matName);

            return OnExit();
            }).value();
        this->RegisterAttachHook(this->hkDrawSceneData, "DrawSceneData");

        this->hkInitDrawingData = MulNX::Hook::Create((uint8_t*)this->pSceneSystem->pInitDrawingData, [this](MulNX::Hook* hk, RegContext* ctx) {
            hk->ResetCallback([this](MulNX::Hook* hk, RegContext* ctx) {return this->OnInitDrawingData(hk, ctx);});

            auto& pDrawingData = *(CS2::DrawingData**)&ctx->rcx;
            auto& pSceneView = *(CS2::CSceneView**)&ctx->rdx;
            auto& pSceneLayer = *(CS2::CSceneLayer**)&ctx->r8;
            auto& unkFlags4 = *(uint32_t*)&ctx->r9;
            auto& pszNameSuffix = *hk->GetStackParam<const char*>(ctx, 4);

            this->pRawInitDrawingData(pDrawingData, pSceneView, pSceneLayer, unkFlags4, pszNameSuffix);
            
            auto pList = pDrawingData->pSoftwareCommandList;
            auto pCommit = pList->GetVPtrCommit();
            this->hkCommit = MulNX::Hook::Create(pCommit, [this](MulNX::Hook* hk, RegContext* ctx) {
                return this->OnCommit(hk, ctx);
                }).value();
            this->RegisterAttachHook(this->hkCommit, "SoftwareCommandList::Commit");
            return MulNX::Hook::Then::Return;
            }).value();
        this->RegisterAttachHook(this->hkInitDrawingData, "InitDrawingData");
        this->pRawInitDrawingData = (SceneSystem::InitDrawingData_t)this->hkInitDrawingData->pMaybeRawFunc;
        });

    return true;
}

MulNX::Hook::Then RendCtrlCenter::OnInitDrawingData(MulNX::Hook* hk, RegContext* ctx) {
    auto& pDrawingData = *(CS2::DrawingData**)&ctx->rcx;
    auto& pSceneView = *(CS2::CSceneView**)&ctx->rdx;
    auto& pSceneLayer = *(CS2::CSceneLayer**)&ctx->r8;
    auto& unkFlags4 = *(uint32_t*)&ctx->r9;
    auto& pszNameSuffix = *hk->GetStackParam<const char*>(ctx, 4);

    this->pRawInitDrawingData(pDrawingData, pSceneView, pSceneLayer, unkFlags4, pszNameSuffix);

    const char* viewPass = pSceneLayer->ViewPass;
    const char* viewName = pSceneView->GetName();

    if (0 == strcmp("Player 0", viewName)) {
        if (0 == strcmp("PostProcessing", viewPass)) {
            int a = 10;
            ++a;
        }
        else if (0 == strcmp("Legacy Sniper Scope", viewPass)) {
            int b = 10;
            ++b;
        }
    }

    return MulNX::Hook::Then::Return;
}

MulNX::Hook::Then RendCtrlCenter::OnCommit(MulNX::Hook* hk, RegContext* ctx) {

    return MulNX::Hook::Then::Continue;
}