#pragma once
#include "IMulNXReShadeBridge.hpp"
#include <atomic>
#include <MulNXThirdParty/reshade/include/reshade.hpp>

class CMulNXReShadeBridge final :public IMulNXReShadeBridge {
    constexpr static const char* implName = MulNXReshadeBridgeVersion;

    struct RtvCache {
        reshade::api::resource      res = { 0 };
        reshade::api::resource_view view = { 0 };
        reshade::api::resource_view view_s = { 0 };
    };
    RtvCache rtvCache;

    struct DepthCache {
        reshade::api::resource texture = { 0 };
        reshade::api::resource_view srv = { 0 };
        reshade::api::resource_desc desc = {};
    };
    DepthCache depthCache;

    std::atomic<bool> enable = false;
    bool inCallEffect = false;
    reshade::api::effect_runtime* pRuntime = nullptr;

    const char* GetReShadeVersion()const override;

    bool InitInterface()override;
    bool DeinitInterface()override;

    bool GetEffectsState()const override;
    void SetEffectsState(bool state)override;

    bool EnsureRtvViews(reshade::api::resource res,
        reshade::api::resource_view& outView, reshade::api::resource_view& outViewSrgb);
    bool EnsureDSVViews(reshade::api::resource depth_res, reshade::api::command_list* cmd);
    bool RendEffect(ID3D11Resource* resource, ID3D11Resource* depthResource)override;
public:
    const char* GetImplName()const override;
    
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