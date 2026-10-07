#include "MulNXReShadeBridge.hpp"
#include <string_view>

static CMulNXReShadeBridge MulNXReShadeBridge;

void CreateInterface(const char* name, IMulNXReShadeBridge** ppInterface) {
    if (std::string_view(CMulNXReShadeBridge::implName) != std::string_view(name))return;
    *ppInterface = &MulNXReShadeBridge;
    return;
}

void RegisterEvents() {
    reshade::register_event<reshade::addon_event::reshade_begin_effects>([](reshade::api::effect_runtime* runtime,
        reshade::api::command_list* cmd_list,
        reshade::api::resource_view rtv,
        reshade::api::resource_view rtv_srgb) {
            MulNXReShadeBridge.OnBeginEffects(runtime, cmd_list, rtv, rtv_srgb);
        });
    
    reshade::register_event<reshade::addon_event::reshade_finish_effects>([](reshade::api::effect_runtime* runtime,
        reshade::api::command_list* cmd_list,
        reshade::api::resource_view rtv,
        reshade::api::resource_view rtv_srgb) {
            MulNXReShadeBridge.OnFinishEffects(runtime, cmd_list, rtv, rtv_srgb);
        });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved) {
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH: {
        if (!reshade::register_addon(hModule)) {
            return FALSE;
        }
        RegisterEvents();
        break;
    }
    case DLL_PROCESS_DETACH:
        reshade::unregister_addon(hModule);
        break;
    }
    return TRUE;
}