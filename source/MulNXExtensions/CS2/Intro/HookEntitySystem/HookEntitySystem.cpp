#include "HookEntitySystem.hpp"

using AddEntity_t = void* (*)(void*, CS2::C_BaseEntity*, CS2::CHandleBase);
using RemoveEntity_t = void* (*)(void*, CS2::C_BaseEntity*, CS2::CHandleBase);

bool HookEntitySystem::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](auto&&...) {
        auto pAddEntity = this->CS2->client.GetTextRegion().FindRegion(CS2::Signatures::Entity::Func_AddEntity).Data();
        this->hkAddEntity = MulNX::Hook::Create(pAddEntity, [this](MulNX::Hook* hk, RegContext* ctx) {
            auto pThis = (void*)ctx->rcx;
            auto pEntity = (CS2::C_BaseEntity*)ctx->rdx;
            auto pHandle = (CS2::CHandleBase*)&(ctx->r8);

            MulNX::Message msg("Hook/AddEntity"_hash);
            auto&& [pMsgEntity, hMsgEntity] = msg.Access<CS2::C_BaseEntity*, CS2::CHandleBase>();
            pMsgEntity = pEntity;
            hMsgEntity = *pHandle;

            // 先调用原函数，确保实体已被添加到系统中，相关数据已初始化
            auto r = reinterpret_cast<AddEntity_t>(hk->pMaybeRawFunc)(pThis, pEntity, *pHandle);
            ctx->rax = reinterpret_cast<uint64_t>(r);

            this->PublishSync(msg);
            
            return MulNX::Hook::Then::Return;
            }) | value_or_throw;
        this->RegisterAttachHook(this->hkAddEntity, "AddEntity");

        auto pRemoveEntity = this->CS2->client.GetTextRegion().FindRegion(CS2::Signatures::Entity::Func_RemoveEntity).Data();
        this->hkRemoveEntity = MulNX::Hook::Create(pRemoveEntity, [this](MulNX::Hook* hk, RegContext* ctx) {
            MulNX::Message msg("Hook/RemoveEntity"_hash);
            auto&& [pEntity] = msg.Access<CS2::C_BaseEntity*>();
            pEntity = (CS2::C_BaseEntity*)(ctx->rdx);
            this->PublishSync(msg);
            return MulNX::Hook::Then::Continue;
            }) | value_or_throw;
        this->RegisterAttachHook(this->hkRemoveEntity, "RemoveEntity");
        });

    return true;
}