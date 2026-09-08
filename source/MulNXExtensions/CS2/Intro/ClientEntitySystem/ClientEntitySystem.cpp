#include "ClientEntitySystem.hpp"
#include <MulNXUtils/WinExt/WinExt.hpp>
#include <Intro/CSController/CSController.hpp>

bool ClientEntitySystem::Init() {
    this->CS2 = this->FindModule<CSController>("CSController");

    return true;
}

CS2::C_BaseEntity* ClientEntitySystem::GetBaseEntity(int index) {
    uintptr_t entListBase = MulNX::MRead<uintptr_t>(this->CS2->client.GetBaseAddress() + cs2_dumper::offsets::client_dll::dwEntityList);
    if (entListBase == 0) {
        return 0;
    }
    uintptr_t entityListBase = MulNX::MRead<uintptr_t>(entListBase + 0x8 * (index >> 9) + 0x10);
    if (entityListBase == 0) {
        return 0;
    }
    return MulNX::MRead<CS2::C_BaseEntity*>(entityListBase + (0x70 * (index & 0x1FF)));
}

CS2::C_BaseEntity* ClientEntitySystem::GetBaseEntityFromHandle(CS2::CHandleBase handle) {
    if (!handle.Valid())return nullptr;
    return this->GetBaseEntity(handle.GetIndexInEntityList());
}

std::optional<std::pair<CS2::CCSPlayerController*, CS2::C_CSPlayerPawn*>> ClientEntitySystem::TryGetPlayer(int index) {
    try {
        auto* entity = this->GetBaseEntity(index);
        if (!entity)return std::nullopt;

        auto* controller = entity->As<CS2::CCSPlayerController>();
        if (!controller->IsPlayerController())return std::nullopt;

        auto hPawn = MulNX::MRead(controller->m_hPlayerPawn());
        auto* pawn = this->GetBaseEntityFromHandle(hPawn)->As<CS2::C_CSPlayerPawn>();
        if (!pawn)return std::nullopt;

        auto team = MulNX::MRead(pawn->iTeamNum());
        if (team != CS2::ui8TeamNum::T && team != CS2::ui8TeamNum::CT)return std::nullopt;

        return std::make_optional(std::make_pair(controller, pawn));
    }
    catch (...) {
        return std::nullopt;
    }
}

CS2::C_CSPlayerPawn* ClientEntitySystem::GetLocalPlayerPawnEx() {
    try {
        auto* localController = this->CS2->client.dwLocalPlayerController();
        if (!localController) return nullptr;
        auto hLocalPawn = MulNX::MRead(localController->m_hPawn());
        auto* localPawn = this->GetBaseEntityFromHandle(hLocalPawn)->As<CS2::C_CSPlayerPawn>();
        return localPawn;
    }
    catch (...) {
        return nullptr;
    }
}

CS2::C_CSPlayerPawn* ClientEntitySystem::TryGetObservingPawn() {
    try {
        auto* localPawn = this->GetLocalPlayerPawnEx();
        if (!localPawn) return nullptr;
        auto hObserverTarget = localPawn->GetHandleObserverTarget();
        auto* target = this->GetBaseEntityFromHandle(hObserverTarget)->As<CS2::C_CSPlayerPawn>();
        return target;
    }
    catch (...) {
        return nullptr;
    }
}

std::optional<Steam64UID> ClientEntitySystem::TryGetObservingSteam64UID() {
    try {
        auto pPawn = this->TryGetObservingPawn();
        if (!pPawn)return std::nullopt;
        auto hController = MulNX::MRead(pPawn->m_hController());
        auto pController = this->GetBaseEntityFromHandle(hController)->As<CS2::CCSPlayerController>();
        if (!pController)return std::nullopt;
        auto steam64 = MulNX::MRead(pController->m_steamID());
        return steam64;
    }
    catch (...) {
        return std::nullopt;
    }
}

CS2::CCSPlayerController* ClientEntitySystem::FindControllerBySteam64UID(Steam64UID uid) {
    try {
        for (int i = 0; i < 32; ++i) {
            auto* controller = this->GetBaseEntity(i)->As<CS2::CCSPlayerController>();
            if (!controller)continue;
            if (!controller->IsPlayerController())continue;
            auto steam64UID = MulNX::MRead(controller->m_steamID());
            if (steam64UID != uid)continue;
            return controller;
        }
    }
    catch (...) {
        this->LogError("FindControllerBySteam64UID 失败");
    }
    return nullptr;
}