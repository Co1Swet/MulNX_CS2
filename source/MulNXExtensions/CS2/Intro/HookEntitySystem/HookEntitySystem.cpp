#include "HookEntitySystem.hpp"

using AddEntity_t = void* (*)(void*, CS2::C_BaseEntity*, CS2::CHandleBase);
using RemoveEntity_t = void* (*)(void*, CS2::C_BaseEntity*, CS2::CHandleBase);

bool HookEntitySystem::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        this->SendTask("DelayInit", "CSControl", [this]()->bool {
            auto gameEntitySystem = this->CS2->client.dwGameEntitySystem();
            if (!gameEntitySystem)return true;// keep task
            auto vtable = (uint8_t**)IVClass::Assume(gameEntitySystem)->GetVTablePtr();
            if (!vtable)MulNX::ErrorTerminate("找不到游戏实体系统的虚函数表，请确认版本适配性");

            auto pAddEntity = vtable[15];
            this->hkAddEntity = MulNX::Hook::Create(pAddEntity, [this](MulNX::Hook* hk, RegContext* ctx) {
                MulNX::Message msg("Hook/AddEntity"_hash);
                auto&& [pEntity, hEntity] = msg.Access<CS2::C_BaseEntity*, CS2::CHandleBase>();
                pEntity = (CS2::C_BaseEntity*)(ctx->rdx);
                hEntity = *(CS2::CHandleBase*)&(ctx->r8);
                hk->CallMaybeOrigin(0, ctx);// 先调用原函数，确保实体已被添加到系统中，相关数据已初始化

                this->PublishSync(msg);
                return MulNX::Hook::Then::Return;
                }).value();
            this->RegisterAttachHook(this->hkAddEntity, "AddEntity");

            auto pRemoveEntity = vtable[16];
            this->hkRemoveEntity = MulNX::Hook::Create(pRemoveEntity, [this](MulNX::Hook* hk, RegContext* ctx) {
                MulNX::Message msg("Hook/RemoveEntity"_hash);
                auto&& [pEntity] = msg.Access<CS2::C_BaseEntity*>();
                pEntity = (CS2::C_BaseEntity*)(ctx->rdx);
                this->PublishSync(msg);
                return MulNX::Hook::Then::Continue;
                }).value();
            this->RegisterAttachHook(this->hkRemoveEntity, "RemoveEntity");

            return false;
            });
        });

    return true;
}