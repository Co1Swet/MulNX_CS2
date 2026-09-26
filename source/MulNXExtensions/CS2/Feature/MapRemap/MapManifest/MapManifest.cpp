#include "MapManifest.hpp"

constexpr std::string_view safeBuf = "models/tools/bullet_hit_marker.vmdl";

bool MapManifest::Init() {
    (*this)
        .SubscribeAsync("MapRemap/Set")
        ;

    // manifest 资源路径
    this->SubscribeSync("Hook/LoadLibraryExW/engine2.dll", [this](auto&&...) {
        auto target = this->CS2->engine2.GetTextRegion().FindRegion(CS2::Signatures::MapRemap::Pos_Manifest_AddFullPath).Data();
        // 表 3：完整资源路径
        this->hkPos_Manifest_AddFullPath = MulNX::Hook::Create(target, [this](MulNX::Hook* hk, RegContext* ctx) {
            auto path = (const char*)ctx->r8;
            if (!path) return MulNX::Hook::Then::Continue;
            std::string_view sv(path);
            std::shared_lock lock(this->smutex);
            if (!this->errorLoadings.contains(std::string(sv)))return MulNX::Hook::Then::Continue;
            ctx->r8 = std::bit_cast<uint64_t>(safeBuf.data());
            return MulNX::Hook::Then::Continue;
            }, true).value();
        this->RegisterAttachHook(this->hkPos_Manifest_AddFullPath, "Pos_Manifest_AddFullPath");
        });

    this->SubscribeSync("Hook/LoadLibraryExW/resourcesystem.dll", [this](auto&&...) {
        auto textRegion = MulNX::Memory::DllModule(L"resourcesystem.dll").GetTextRegion();
        auto t = textRegion.FindRegion(CS2::Signatures::MapRemap::Pos_Log_Failedloading).Data();
        this->hkPos_Log_Failedloading = MulNX::Hook::Create(t, [this](MulNX::Hook* hk, RegContext* ctx) {
            auto pPath = std::bit_cast<const char*>(ctx->rax);
            auto path = std::string_view(pPath);
            if (!path.ends_with(".vmdl_c"))return MulNX::Hook::Then::Continue;
            path = path.substr(0, path.size() - 2);
            std::unique_lock lock(this->smutex);
            this->errorLoadings.insert(std::string(path));
            return MulNX::Hook::Then::Continue;
            }, true).value();
        this->RegisterAttachHook(this->hkPos_Log_Failedloading, "Pos_Log_Failedloading");
        });

    this->SendTask("Update", "CSControl", [this]() {
        this->Update();
        return true;
        });

    return true;
}

void MapManifest::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "MapRemap/Set"_hash: {
        std::unique_lock lock(this->smutex);
        this->errorLoadings.clear();
        this->LogInfo("错误拦截列表已清空");
        break;
    }
    }
}