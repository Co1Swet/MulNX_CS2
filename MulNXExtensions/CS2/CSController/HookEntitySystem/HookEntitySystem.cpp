#include "HookEntitySystem.hpp"

#include <MulNXExtensions/CS2/CSController/CSController.hpp>

using AddEntity_t = void* (*)(void*, CS2::C_BaseEntity*, CS2::CHandleBase);
using RemoveEntity_t = void* (*)(void*, CS2::C_BaseEntity*, CS2::CHandleBase);

bool HookEntitySystem::Init() {
    this->ISys().SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        this->ISys().SendTask("DelayInit", "CSControl", [this]()->bool {
            auto gameEntitySystem = this->CS2()->client.dwGameEntitySystem();
            if (!gameEntitySystem)return true;// keep task
            auto vtable = (uint8_t**)IVClass::Assume(gameEntitySystem)->GetVTablePtr();

            auto pAddEntity = vtable[15];
            this->hkAddEntity = MulNX::Hook::Create(pAddEntity, [this](MulNX::Hook* hk, RegContext* ctx) {
                CS2::C_BaseEntity* pEntity = (CS2::C_BaseEntity*)(ctx->rdx);
                CS2::CHandleBase hEntity = *(CS2::CHandleBase*)&(ctx->r8);

                MulNX::Message msg("Game/Entity/Added"_hash);
                msg.p1.as<CS2::C_BaseEntity*>() = pEntity;
                msg.p2.low<CS2::CHandleBase>() = hEntity;
                this->ISys().PublishAsync(std::move(msg));
                return MulNX::Hook::Then::Continue;
                }).value();
            this->hkAddEntity->Attach();
            this->ISys().LogSucc(I18n("hook.attached", "AddEntity"));

            auto pRemoveEntity = vtable[16];
            this->hkRemoveEntity = MulNX::Hook::Create(pRemoveEntity, [this](MulNX::Hook* hk, RegContext* ctx) {
                auto pEntity = (CS2::C_BaseEntity*)(ctx->rdx);
                MulNX::Message msg("Game/Entity/Removed"_hash);
                msg.p1.as<CS2::C_BaseEntity*>() = pEntity;
                this->ISys().PublishAsync(std::move(msg));
                return MulNX::Hook::Then::Continue;
                }).value();
            this->hkRemoveEntity->Attach();
            this->ISys().LogSucc(I18n("hook.attached", "RemoveEntity"));

            return false;
            });
        });

    return true;
}