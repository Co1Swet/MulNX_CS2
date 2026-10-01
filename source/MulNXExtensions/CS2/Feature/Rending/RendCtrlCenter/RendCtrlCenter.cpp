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
        });

    return true;
}