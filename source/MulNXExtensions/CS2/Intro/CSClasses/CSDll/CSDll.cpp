#include "CSDll.hpp"

CS2::C_BaseEntity* CS2::Module::Client::GetBaseEntity(int index) {
    uintptr_t entListBase = MulNX::MRead<uintptr_t>(this->GetBaseAddress() + cs2_dumper::offsets::client_dll::dwEntityList);
    if (entListBase == 0) {
        return 0;
    }
    uintptr_t entityListBase = MulNX::MRead<uintptr_t>(entListBase + 0x8 * (index >> 9) + 0x10);
    if (entityListBase == 0) {
        return 0;
    }
    return MulNX::MRead<CS2::C_BaseEntity*>(entityListBase + (0x70 * (index & 0x1FF)));
}

CS2::C_BaseEntity* CS2::Module::Client::GetBaseEntityFromHandle(CS2::CHandleBase handle) {
    if (!handle.Valid())return nullptr;
    return this->GetBaseEntity(handle.GetIndexInEntityList());
}

CS2::C_CSPlayerPawn* CS2::Module::Client::GetLocalPlayerPawn() {
    try {
        auto* localController = this->dwLocalPlayerController();
        if (!localController) return nullptr;
        auto hLocalPawn = MulNX::MRead(localController->m_hPawn());
        auto* localPawn = this->GetBaseEntityFromHandle(hLocalPawn)->As<CS2::C_CSPlayerPawn>();
        return localPawn;
    }
    catch (...) {
        return nullptr;
    }
}

CS2::C_CSPlayerPawn* CS2::Module::Client::TryGetObservingPawn() {
    try {
        auto* localPawn = this->GetLocalPlayerPawn();
        if (!localPawn) return nullptr;
        auto hObserverTarget = localPawn->GetHandleObserverTarget();
        auto* target = this->GetBaseEntityFromHandle(hObserverTarget)->As<CS2::C_CSPlayerPawn>();
        return target;
    }
    catch (...) {
        return nullptr;
    }
}

std::optional<Steam64UID> CS2::Module::Client::TryGetObservingSteam64UID() {
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