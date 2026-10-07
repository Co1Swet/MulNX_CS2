#pragma once
#include <d3d11.h>

class IMulNXReShadeBridge {
public:
    virtual const char* GetImplName()const = 0;
    virtual const char* GetReShadeVersion()const = 0;

    virtual bool InitInterface() = 0;
    virtual bool DeinitInterface() = 0;

    virtual bool GetEffectsState()const = 0;
    virtual void SetEffectsState(bool state) = 0;

    virtual bool RendEffect(ID3D11Resource* resource) = 0;
};

extern "C" __declspec(dllexport) void CreateInterface(const char* name, IMulNXReShadeBridge** ppInterface);