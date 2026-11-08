#include "MulNXReShadeBridge.hpp"

const char* CMulNXReShadeBridge::GetImplName()const {
    return this->implName;
}
const char* CMulNXReShadeBridge::GetReShadeVersion()const {
    return nullptr;
}

bool CMulNXReShadeBridge::InitInterface() {
    return true;
}
bool CMulNXReShadeBridge::DeinitInterface() {
    return true;
}

bool CMulNXReShadeBridge::GetEffectsState()const {
    return this->enable.load(std::memory_order_acquire);
}
void CMulNXReShadeBridge::SetEffectsState(bool state) {
    this->enable.store(state, std::memory_order_release);
}

bool CMulNXReShadeBridge::EnsureRtvViews(reshade::api::resource res,
    reshade::api::resource_view& outView, reshade::api::resource_view& outViewSrgb) {

    reshade::api::device* dev = this->pRuntime->get_device();
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
bool CMulNXReShadeBridge::EnsureDSVViews(reshade::api::resource depth_res, reshade::api::command_list* cmd) {
    if (depth_res.handle == 0)return false;

    reshade::api::device* device = this->pRuntime->get_device();
    if (!device)return false;

    auto depth_desc = device->get_resource_desc(depth_res);

    if (this->depthCache.texture.handle != 0 &&
        (this->depthCache.desc.type != depth_desc.type ||
            this->depthCache.desc.texture.width != depth_desc.texture.width ||
            this->depthCache.desc.texture.height != depth_desc.texture.height ||
            this->depthCache.desc.texture.depth_or_layers != depth_desc.texture.depth_or_layers ||
            this->depthCache.desc.texture.levels != depth_desc.texture.levels ||
            this->depthCache.desc.texture.format != depth_desc.texture.format ||
            this->depthCache.desc.texture.samples != depth_desc.texture.samples)) {
        if (this->depthCache.srv.handle != 0)
            device->destroy_resource_view(this->depthCache.srv);
        device->destroy_resource(this->depthCache.texture);
        this->depthCache = {};
    }

    if (this->depthCache.texture.handle == 0) {
        this->depthCache.desc = depth_desc;
        this->depthCache.desc.usage = reshade::api::resource_usage::shader_resource |
            reshade::api::resource_usage::copy_dest;

        if (!device->create_resource(this->depthCache.desc, nullptr,
            reshade::api::resource_usage::copy_dest, &this->depthCache.texture)) {
            this->depthCache = {};
            return false;
        }

        reshade::api::resource_view_desc view_desc
        (reshade::api::format_to_default_typed(this->depthCache.desc.texture.format));
        if (!device->create_resource_view(this->depthCache.texture,
            reshade::api::resource_usage::shader_resource, view_desc, &this->depthCache.srv)) {
            device->destroy_resource(this->depthCache.texture);
            this->depthCache = {};
            return false;
        }
    }

    cmd->barrier(depth_res, reshade::api::resource_usage::depth_stencil, reshade::api::resource_usage::copy_source);

    if (depth_desc.texture.samples > 1) {
        cmd->resolve_texture_region(depth_res, 0, nullptr, this->depthCache.texture,
            0, 0, 0, 0, this->depthCache.desc.texture.format);
    }
    else {
        cmd->copy_texture_region(depth_res, 0, nullptr, this->depthCache.texture, 0, nullptr);
    }

    cmd->barrier(this->depthCache.texture, reshade::api::resource_usage::copy_dest,
        reshade::api::resource_usage::shader_resource);
    this->pRuntime->update_texture_bindings("DEPTH", this->depthCache.srv, this->depthCache.srv);

    this->pRuntime->enumerate_uniform_variables(nullptr, [this](reshade::api::effect_runtime* runtime, auto variable) {
        char source[32] = "";
        if (runtime->get_annotation_string_from_uniform_variable(variable, "source", source) &&
            std::strcmp(source, "bufready_depth") == 0)
            runtime->set_uniform_value_bool(variable, this->depthCache.srv.handle != 0);
        });
    return true;
}

bool CMulNXReShadeBridge::RendEffect(ID3D11Resource* resource, ID3D11Resource* depthResource) {
    if (!this->pRuntime || !resource)return false;

    reshade::api::resource res{ reinterpret_cast<uint64_t>(resource) };
    reshade::api::resource depth_res{ reinterpret_cast<uint64_t>(depthResource) };

    reshade::api::command_queue* queue = this->pRuntime->get_command_queue();
    if (!queue)return false;
    reshade::api::command_list* cmd = queue->get_immediate_command_list();
    if (!cmd)return false;

    reshade::api::resource_view rtv{ 0 }, rtv_srgb{ 0 };
    if (!this->EnsureRtvViews(res, rtv, rtv_srgb))return false;
    if (!this->EnsureDSVViews(depth_res, cmd)) {
        this->pRuntime->update_texture_bindings("DEPTH", { 0 }, { 0 });
        return false;
    }

    this->pRuntime->set_effects_state(true);
    this->inCallEffect = true;
    this->pRuntime->render_effects(cmd, rtv, rtv_srgb);
    this->inCallEffect = false;

    return true;
}

void CMulNXReShadeBridge::OnBeginEffects(reshade::api::effect_runtime* runtime,
    reshade::api::command_list* cmd_list,
    reshade::api::resource_view rtv,
    reshade::api::resource_view rtv_srgb) {

    runtime->set_effects_state(this->enable.load(std::memory_order_acquire));
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