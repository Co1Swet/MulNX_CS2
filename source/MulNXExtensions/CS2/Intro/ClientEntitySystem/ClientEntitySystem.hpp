#pragma once
#include <MulNX/MulNX.hpp>
#include <Game/tree/tree.hpp>

class CSController;
class ClientEntitySystem final :public MulNX::Module<ClientEntitySystem> {
    CSController* CS2 = nullptr;

    bool Init()override;
public:
    CS2::C_BaseEntity* GetBaseEntity(int index);
    CS2::C_BaseEntity* GetBaseEntityFromHandle(CS2::CHandleBase handle);

    CS2::C_CSPlayerPawn* GetLocalPlayerPawnEx();

    CS2::C_CSPlayerPawn* TryGetObservingPawn();
    std::optional<Steam64UID> TryGetObservingSteam64UID();

    CS2::CCSPlayerController* FindControllerBySteam64UID(Steam64UID uid);
};