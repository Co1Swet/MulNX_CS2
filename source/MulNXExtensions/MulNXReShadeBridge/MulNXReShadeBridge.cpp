#include "MulNXReShadeBridge.hpp"

const char* CMulNXReShadeBridge::GetImplName() {
    return this->implName;
}
const char* CMulNXReShadeBridge::GetReShadeVersion() {
    return nullptr;
}

bool CMulNXReShadeBridge::InitInterface() {
    return true;
}
bool CMulNXReShadeBridge::DeinitInterface() {
    return true;
}

bool CMulNXReShadeBridge::GetEffectsState() {
    return true;
}
void CMulNXReShadeBridge::SetEffectsState(bool state) {
    this->reshadeEffect.store(state, std::memory_order_release);
}


void CMulNXReShadeBridge::OnBeginEffects(reshade::api::effect_runtime* runtime,
    reshade::api::command_list* cmd_list,
    reshade::api::resource_view rtv,
    reshade::api::resource_view rtv_srgb) {

    runtime->set_effects_state(this->reshadeEffect.load(std::memory_order_acquire));
}

void CMulNXReShadeBridge::OnFinishEffects(reshade::api::effect_runtime* runtime,
    reshade::api::command_list* cmd_list,
    reshade::api::resource_view rtv,
    reshade::api::resource_view rtv_srgb) {

    runtime->set_effects_state(true);
}