#include "MapManifest.hpp"

bool MapManifest::Init() {
    // manifest 资源路径
    this->SubscribeSync("Hook/LoadLibraryExW/engine2.dll", [this](auto&&...) {
        auto target = this->CS2->engine2.GetTextRegion().FindRegion(MulNX::CS2::Signatures::MapRemap::Pos_Manifest_AddFullPath).Data();
        // 表 3：完整资源路径
        this->hkPos_Manifest_AddFullPath = MulNX::Hook::Create(target, [this](MulNX::Hook* hk, RegContext* ctx) {
            auto path = (const char*)ctx->r8;
            if (!path) return MulNX::Hook::Then::Continue;
            std::string_view sv(path);
            if (kMissingEntities.contains(sv)) {
                // 替换
                static thread_local char safeBuf[] = "models/tools/bullet_hit_marker.vmdl";
                ctx->r8 = (uint64_t)safeBuf;
                this->LogInfo(std::format("替换失败实体: {} -> models/tools/bullet_hit_marker.vmdl", path));
            }
            else {
                //this->LogInfo(std::format("[Manifest/Path] {}", path));
            }
            return MulNX::Hook::Then::Continue;
            }, true).value();
        this->RegisterAttachHook(this->hkPos_Manifest_AddFullPath, "Pos_Manifest_AddFullPath");
        });

    return true;
}