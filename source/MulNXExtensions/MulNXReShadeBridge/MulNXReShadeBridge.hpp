#pragma once
#include "IMulNXReShadeBridge.hpp"
#include <atomic>
#include <MulNXThirdParty/reshade/include/reshade.hpp>

class CMulNXReShadeBridge final :public IMulNXReShadeBridge {
public:
    constexpr static const char* implName = "MulNXReShadeBridge001";
private:
    std::atomic<bool> reshadeEffect = false;
    
    const char* GetImplName()override;
    const char* GetReShadeVersion()override;

    bool InitInterface()override;
    bool DeinitInterface()override;

    bool GetEffectsState()override;
    void SetEffectsState(bool state)override;
public:
    void OnBeginEffects(reshade::api::effect_runtime* runtime,
        reshade::api::command_list* cmd_list,
        reshade::api::resource_view rtv,
        reshade::api::resource_view rtv_srgb);

    void OnFinishEffects(reshade::api::effect_runtime* runtime,
        reshade::api::command_list* cmd_list,
        reshade::api::resource_view rtv,
        reshade::api::resource_view rtv_srgb);
};