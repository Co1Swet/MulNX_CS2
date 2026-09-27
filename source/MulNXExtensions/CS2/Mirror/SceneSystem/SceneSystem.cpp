#include "SceneSystem.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <Mirror/MaterialSystem/MaterialSystem.hpp>

class IRenderThreadCallback {
public:
    virtual void OnCallback(void) = 0;
};

class CMyMinimalCallback {
public:
    virtual void OnCallback(void) {
        delete this;
    }
};

namespace CS2 {
    class CBaseSceneData {
    public:
        char pad0[0x18];
        void* sceneObject;
        CMaterial2* material;
        char pad1[0x28];
        uint32_t color;
        char pad2[0x14];
    };

    class SoftwareCommandList {
        using QueueCallbackFn = void(__fastcall*)(void* pThis, void* pCallback);
    public:
        void QueueCallback(void* pCallback) {
            auto f = IVClass::Assume(this)->GetVFunc<void(void*)>(142);
            f(pCallback);
        }
    };

    class DrawingData {
    public:
        std::byte pad[0x20];
        CS2::SoftwareCommandList* pSoftwareCommandList;
    };
}

bool SceneSystem::Init() {
    this->pMaterialSystem = this->FindModule<MaterialSystem>("MaterialSystem");

    this->SubscribeSync("Hook/LoadLibraryExW/scenesystem.dll", [this](MulNX::Message& msg) {
        this->scenesystem = MulNX::Memory::DllModule(L"scenesystem.dll");

        auto tDrawSceneData = this->scenesystem.GetTextRegion().FindRegion(CS2::Signatures::SceneSystem::Func_DrawSceneData).Data();
        this->hkDrawSceneData = MulNX::Hook::Create(tDrawSceneData, [this](MulNX::Hook* hk, RegContext* ctx) {
            auto* pDrawingData = (CS2::DrawingData*)ctx->rcx;
            auto* pSceneData = (CS2::CBaseSceneData*)ctx->rdx;

            if (pSceneData->material) {
                const char* matName = pSceneData->material->GetName();
                if (matName && (strstr(matName, "clouds") || strstr(matName, "sun_disc_glow"))) {
                    pDrawingData->pSoftwareCommandList->QueueCallback(new CMyMinimalCallback());
                }
            }
            

            return MulNX::Hook::Then::Continue;
            }).value();
        this->RegisterAttachHook(this->hkDrawSceneData, "DrawSceneData");
        });

    return true;
}