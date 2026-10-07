#pragma once
#include "IMulNXReShadeBridge.hpp"
#include <atomic>
#include <MulNXThirdParty/reshade/include/reshade.hpp>

class CMulNXReShadeBridge final :public IMulNXReShadeBridge {
    struct RtvCache {
        reshade::api::resource      res = { 0 };
        reshade::api::resource_view view = { 0 };
        reshade::api::resource_view view_s = { 0 };
    };
    RtvCache rtvCache;
    bool EnsureRtvViews(reshade::api::resource res,
        reshade::api::resource_view& outView, reshade::api::resource_view& outViewSrgb);
public:
    constexpr static const char* implName = "MulNXReShadeBridge001";
private:
    std::atomic<bool> reshadeEffect = false;
    bool inCallEffect = false;
    reshade::api::effect_runtime* pRuntime = nullptr;

    const char* GetImplName()override;
    const char* GetReShadeVersion()override;

    bool InitInterface()override;
    bool DeinitInterface()override;

    bool GetEffectsState()override;
    void SetEffectsState(bool state)override;

    bool RendEffect(ID3D11Resource* resource)override;
public:
    void OnBeginEffects(reshade::api::effect_runtime* runtime,
        reshade::api::command_list* cmd_list,
        reshade::api::resource_view rtv,
        reshade::api::resource_view rtv_srgb);

    void OnFinishEffects(reshade::api::effect_runtime* runtime,
        reshade::api::command_list* cmd_list,
        reshade::api::resource_view rtv,
        reshade::api::resource_view rtv_srgb);

    void OnPresent(reshade::api::effect_runtime* runtime);
};