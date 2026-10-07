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

bool CMulNXReShadeBridge::EnsureRtvViews(reshade::api::resource res,
    reshade::api::resource_view& outView, reshade::api::resource_view& outViewSrgb) {

    reshade::api::device* dev = pRuntime->get_device();
    if (!dev) return false;
    
    if (this->rtvCache.res == res && this->rtvCache.view.handle != 0 && this->rtvCache.view_s.handle != 0) {
        outView = this->rtvCache.view;
        outViewSrgb = this->rtvCache.view_s;
        return true;
    }

    if (this->rtvCache.view.handle != 0) dev->destroy_resource_view(this->rtvCache.view);
    if (this->rtvCache.view_s.handle != 0) dev->destroy_resource_view(this->rtvCache.view_s);
    this->rtvCache = {};

    const reshade::api::resource_desc desc = dev->get_resource_desc(res);

    if (!dev->create_resource_view(res,
        reshade::api::resource_usage::render_target,
        reshade::api::resource_view_desc(
            reshade::api::format_to_default_typed(desc.texture.format, 0)),
        &this->rtvCache.view))
        return false;

    if (!dev->create_resource_view(res,
        reshade::api::resource_usage::render_target,
        reshade::api::resource_view_desc(
            reshade::api::format_to_default_typed(desc.texture.format, 1)),
        &this->rtvCache.view_s)) {
        dev->destroy_resource_view(this->rtvCache.view);
        this->rtvCache = {};
        return false;
    }

    this->rtvCache.res = res;
    outView = this->rtvCache.view;
    outViewSrgb = this->rtvCache.view_s;
    return true;
}

bool CMulNXReShadeBridge::RendEffect(ID3D11Resource* resource) {
    if (!pRuntime || !resource) return false;

    reshade::api::resource res{ reinterpret_cast<uint64_t>(resource) };

    reshade::api::command_queue* queue = pRuntime->get_command_queue();
    if (!queue) return false;

    reshade::api::command_list* cmd = queue->get_immediate_command_list();
    if (!cmd) return false;

    reshade::api::resource_view rtv{ 0 }, rtv_srgb{ 0 };
    if (!this->EnsureRtvViews(res, rtv, rtv_srgb)) return false;

    pRuntime->set_effects_state(true);

    this->inCallEffect = true;
    pRuntime->render_effects(cmd, rtv, rtv_srgb);
    this->inCallEffect = false;

    return true;
}


void CMulNXReShadeBridge::OnBeginEffects(reshade::api::effect_runtime* runtime,
    reshade::api::command_list* cmd_list,
    reshade::api::resource_view rtv,
    reshade::api::resource_view rtv_srgb) {
    if (this->inCallEffect) {
        runtime->set_effects_state(true);
        return;
    }
    runtime->set_effects_state(this->reshadeEffect.load(std::memory_order_acquire));
}

void CMulNXReShadeBridge::OnFinishEffects(reshade::api::effect_runtime* runtime,
    reshade::api::command_list* cmd_list,
    reshade::api::resource_view rtv,
    reshade::api::resource_view rtv_srgb) {

    runtime->set_effects_state(true);
}

void CMulNXReShadeBridge::OnPresent(reshade::api::effect_runtime* runtime) {
    this->pRuntime = runtime;
}