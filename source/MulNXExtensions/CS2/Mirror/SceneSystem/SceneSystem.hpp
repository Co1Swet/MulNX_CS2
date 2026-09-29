#pragma once
#include <Intro/CSModuleBase.hpp>

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

class WrapCS2RenderCallback final :public CS2::IRenderThreadCallback {
    std::function<void(void)> callback{};

    ~WrapCS2RenderCallback() = default;
public:
    WrapCS2RenderCallback(std::function<void(void)>&& call) {
        this->callback = std::move(call);
    }
    virtual void OnCallback(void)override {
        this->callback();
        delete this;
    }
};

class SceneSystem final :public CSModuleBase {
    MulNX::Memory::DllModule scenesystem{};
    
    bool Init()override;
public:
    using DrawCurrentPrimitives_t = void(*)(CS2::DrawingData* pDrawingData);
    DrawCurrentPrimitives_t pDrawCurrentPrimitives = nullptr;

    using DrawSceneData_t = void(*)(CS2::DrawingData* pDrawingData, CS2::CBaseSceneData* pSceneData);
    DrawSceneData_t pDrawSceneData = nullptr;
};