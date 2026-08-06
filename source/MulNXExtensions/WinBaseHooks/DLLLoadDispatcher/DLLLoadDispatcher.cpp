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
    this->RegisterAttachHook(this->hkLoadLibraryExW, "LoadLibraryExW");

    auto pthFile = this->PathGet("Config") / "dllTargets.yaml";
    auto file= YAML::LoadFile(pthFile.string());
    for (const auto& target : file["targets"]) {
        this->targets.insert(target.as<std::string>());
        this->LogInfo(std::format("已添加拦截目标：{}", target.as<std::string>()));
    }
    return true;
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
        this->LogWarning(std::format("检测到目标模块加载：{}", filename));
        auto full = "Hook/LoadLibraryExW/" + filename;
        this->PublishSync(MulNX::HashString(full));
    }
}