#include "DLLLoadDispatcher.hpp"

bool DLLLoadDispatcher::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW", [this](MulNX::Message& msg) {this->OnModuleLoaded(msg); });

    this->hkLoadLibraryExW = MulNX::Hook::Create((uint8_t*)LoadLibraryExW, [this](MulNX::Hook* hk, RegContext* ctx) {
        MulNX::Message msg("Hook/LoadLibraryExW"_hash);
        auto&& [lpLibFileName] = msg.Access<LPCWSTR>();
        lpLibFileName = (LPCWSTR)ctx->rcx;
        hk->CallMaybeOrigin(0, ctx);
        this->PublishSync(msg);
        return MulNX::Hook::Then::Return;
        }).value();
    this->hkLoadLibraryExW->Attach();
    this->LogSucc(I18n("hook.attached", "LoadLibraryExW"));

    auto pthFile = this->PathGet("Config") / "dllTargets.yaml";
    auto file= YAML::LoadFile(pthFile.string());
    for (const auto& target : file["targets"]) {
        this->targets.insert(target.as<std::string>());
    }
    return true;
}
void DLLLoadDispatcher::Deinit() {
    this->hkLoadLibraryExW->Detach();
}

void DLLLoadDispatcher::OnModuleLoaded(MulNX::Message& msg) {
    auto&& [lpLibFileName] = msg.Access<LPCWSTR>();
    std::filesystem::path fsPath(lpLibFileName);
    std::unique_lock lock(this->smutex);
    if (this->loadedModules.find(fsPath) == this->loadedModules.end()) {
        this->loadedModules.insert(fsPath);
        lock.unlock();
        this->DispatchModuleLoadMessage(fsPath);
    }
}

void DLLLoadDispatcher::DispatchModuleLoadMessage(const std::filesystem::path& modulePath) {
    auto filename = modulePath.filename().string();
    if (this->targets.find(filename) != this->targets.end()) {
        auto full = "Hook/LoadLibraryExW/" + filename;
        this->PublishSync(MulNX::HashString(full));
    }
}