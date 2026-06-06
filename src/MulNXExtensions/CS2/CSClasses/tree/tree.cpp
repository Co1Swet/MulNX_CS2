#include "tree.hpp"

#include <MulNXExtensions/WinExt/Memory/Memory.hpp>

DirectX::XMFLOAT3 CS2::C_BaseEntity::GetBonePos(int index) {
    auto* pGameSceneNode = MulNX::MRead(this->pGameSceneNode());
    if (!pGameSceneNode) return{};
    auto* Bones = MulNX::MRead(static_cast<CS2::CSkeletonInstance*>(pGameSceneNode)->unkBoneArray());
    if (!Bones) return{};
    DirectX::XMFLOAT3 Pos = MulNX::MRead(Bones->at(index));
    return Pos;
}

std::string CS2::C_BaseEntity::GetName() {
    auto* pClassInfo = MulNX::MRead(this->pClassInfo());
    if (!pClassInfo) return{};
    auto* pName= MulNX::MRead(pClassInfo->pName());
    if (!pName) return{};
    auto className = MulNX::Memory::ReadString(pName);
    return className;
}

bool CS2::C_BaseEntity::IsPlayerController() {
    return this->GetName() == "cs_player_controller";
}
bool CS2::C_BaseEntity::IsPlayerPawn() {
    return this->GetName() == "c_cs_player_for_precache" || this->GetName() == "c_cs_observer_for_precache";
}

CS2::CHandle<CS2::C_BasePlayerWeapon> CS2::C_BasePlayerPawn::GetHandleActiveWeapon() {
    auto* pWeaponServices = MulNX::MRead(this->pWeaponServices());
    if (!pWeaponServices) return {};
    auto hActiveWeapon = MulNX::MRead(pWeaponServices->hActiveWeapon());
    return hActiveWeapon;
}

CS2::CHandle<CS2::C_BaseEntity> CS2::C_BasePlayerPawn::GetHandleObserverTarget() {
    auto* pObserverServices = MulNX::MRead(this->pObserverServices());
    if (!pObserverServices) return{};
    auto hObserverTarget = MulNX::MRead(pObserverServices->hObserverTarget());
    return hObserverTarget;
}