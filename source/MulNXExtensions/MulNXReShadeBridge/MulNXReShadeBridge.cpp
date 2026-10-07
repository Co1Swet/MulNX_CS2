#include "MulNXReShadeBridge.hpp"
#include <MulNXThirdParty/reshade/include/reshade.hpp>

// ReShade 即将自动跑 effects —— 就是这里拦
static void OnBeginEffects(reshade::api::effect_runtime* runtime,
    reshade::api::command_list* /*cmd_list*/,
    reshade::api::resource_view /*rtv*/,
    reshade::api::resource_view /*rtv_srgb*/) {
    // 关掉本次 effects 执行
    runtime->set_effects_state(false);
}

static void OnFinishEffects(reshade::api::effect_runtime* runtime,
    reshade::api::command_list*,
    reshade::api::resource_view,
    reshade::api::resource_view) {
    // 如果这一帧被我们拦了，且原本是开的，就恢复回去
    runtime->set_effects_state(true);
}

// 阻止用户用 ReShade 快捷键切换 effects
static bool OnSetEffectsState(reshade::api::effect_runtime* /*runtime*/, bool /*enabled*/) {
    return false; // false = 拦截默认行为，不执行切换
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved) {
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH: {
        if (!reshade::register_addon(hModule)) {
            // ReShade 未加载或版本不兼容
            return FALSE;
        }

        reshade::register_event<reshade::addon_event::reshade_begin_effects>(OnBeginEffects);
        reshade::register_event<reshade::addon_event::reshade_finish_effects>(OnFinishEffects);
        reshade::register_event<reshade::addon_event::reshade_set_effects_state>(OnSetEffectsState);
        break;
    }
    case DLL_PROCESS_DETACH:
        reshade::unregister_addon(hModule);
        break;
    }
    return TRUE;
}