#include "HookEntitySystem.hpp"

#include <MulNXExtensions/CS2/CSController/CSController.hpp>

using AddEntity_t = void* (*)(void* , CS2::C_BaseEntity* , CS2::CHandleBase);
using RemoveEntity_t = void* (*)(void* , CS2::C_BaseEntity* , CS2::CHandleBase);

bool HookEntitySystem::Init() {
    auto vtable = (uint8_t**)IVClass::Assume(this->CS2()->Modules.client.dwGameEntitySystem())->GetVTablePtr();

    auto pAddEntity = vtable[15];
    this->hkAddEntity = MulNX::Hook::Create(pAddEntity,
        0, false, [this](RegContext* ctx, MulNX::Hook* hk) {
            CS2::C_BaseEntity* pEntity = *ctx->P2<CS2::C_BaseEntity*>();
            CS2::CHandleBase hEntity = *ctx->P3<CS2::CHandleBase>();

            MulNX::Message msg("Game/Entity/Added"_hash);
            msg.p1.as<CS2::C_BaseEntity*>() = pEntity;
            msg.p2.low<CS2::CHandleBase>() = hEntity;
            this->ISys().PublishAsync(std::move(msg));
            return MulNX::Hook::Then::Continue;
        }).value();
    this->hkAddEntity->Attach();
    this->ISys().LogSucc(I18n("hook.attached", "AddEntity"));

    auto pRemoveEntity = vtable[16];
    this->hkRemoveEntity = MulNX::Hook::Create(pRemoveEntity,
        0, false, [this](RegContext* ctx, MulNX::Hook* hk) {
            auto pEntity = *ctx->P2<CS2::C_BaseEntity*>();
            MulNX::Message msg("Game/Entity/Removed"_hash);
            msg.p1.as<CS2::C_BaseEntity*>() = pEntity;
            this->ISys().PublishAsync(std::move(msg));
            return MulNX::Hook::Then::Continue;
        }).value();
    this->hkRemoveEntity->Attach();
    this->ISys().LogSucc(I18n("hook.attached", "RemoveEntity"));

    return true;
}