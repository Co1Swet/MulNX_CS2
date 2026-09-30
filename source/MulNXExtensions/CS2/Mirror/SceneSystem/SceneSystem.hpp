#pragma once
#include <Intro/CSModuleBase.hpp>
#include <d3d11.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

namespace CS2 {
    class IRenderThreadCallback {
    public:
        virtual void OnCallback(void) = 0;
    };

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
        using QueueCallbackFn = void(*)(SoftwareCommandList* pThis, IRenderThreadCallback* pCallback);
    public:
        void QueueCallback(IRenderThreadCallback* pCallback) {
            auto f = IVClass::Assume(this)->GetVFunc<void(IRenderThreadCallback*)>(142);
            f(pCallback);
        }
    };

    class DrawingData {
    public:
        std::byte pad[0x20];
        CS2::SoftwareCommandList* pSoftwareCommandList;
    };
}

class SceneSystem final :public CSModuleBase {
    MulNX::Memory::DllModule scenesystem{};
    
    bool Init()override;
public:
    using DrawCurrentPrimitives_t = void(*)(CS2::DrawingData* pDrawingData);
    DrawCurrentPrimitives_t pDrawCurrentPrimitives = nullptr;

    using DrawSceneData_t = void(*)(CS2::DrawingData* pDrawingData, CS2::CBaseSceneData* pSceneData);
    DrawSceneData_t pDrawSceneData = nullptr;
};