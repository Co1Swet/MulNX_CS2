#include "MapResource.hpp"

bool MapResource::Init() {
    constexpr uint64_t kVmdlTypeId = 0x6C646D76ULL;

    this->SubscribeSync("Hook/LoadLibraryExW/resourcesystem.dll", [this](auto&&...) {
        auto textRegion = MulNX::Memory::DllModule(L"resourcesystem.dll").GetTextRegion();
        auto tFunc_RequestResourceByHash = textRegion.
            FindRegion(CS2::Signatures::MapRemap::Func_RequestResourceByHash).Data();
        this->pFindByHash = std::bit_cast<FindResourceByHash_t>(textRegion.
            FindRegion(CS2::Signatures::MapRemap::Func_FindResourceByHash).Data());

        this->hkFunc_RequestResourceByHash = MulNX::Hook::Create(tFunc_RequestResourceByHash,
            [this](MulNX::Hook* hk, RegContext* ctx) {
                uint64_t pRS = ctx->rcx;
                uint64_t hash = ctx->rdx;
                uint64_t type = ctx->r8;
                // 检验请求是否是我们想控制的
                if (type != kVmdlTypeId || hash == 0)return MulNX::Hook::Then::Continue;
                // 检验 hash 是否可达
                uint64_t node = this->pFindByHash(pRS + 2808 * 4, hash);
                if (node != 0)return MulNX::Hook::Then::Continue;
                ctx->rax = 0;
                return MulNX::Hook::Then::Return;
            }).value();
        this->RegisterAttachHook(this->hkFunc_RequestResourceByHash, "Func_RequestResourceByHash");
        });

    return true;
}