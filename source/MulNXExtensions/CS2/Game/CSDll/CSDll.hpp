#pragma once
#include <MulNX/Config/Config.hpp>
#include <MulNXUtils/WinExt/WinExt.hpp>
#include <Game/tree/tree.hpp>
#include <Game/C_CSGameRules/C_CSGameRules.hpp>

namespace CS2 {
    namespace Module {
        class Client :public MulNX::Memory::DllModule {
        public:
            using MulNX::Memory::DllModule::DllModule;
            uintptr_t dwEntityList() { return MulNX::MRead<uintptr_t>(this->GetBaseAddress() + cs2_dumper::offsets::client_dll::dwEntityList); }
            uintptr_t dwGameEntitySystem() { return MulNX::MRead<uintptr_t>(this->GetBaseAddress() + cs2_dumper::offsets::client_dll::dwGameEntitySystem); }
            int dwGameEntitySystem_highestEntityIndex() { return MulNX::MRead<int>(this->dwGameEntitySystem() + cs2_dumper::offsets::client_dll::dwGameEntitySystem_highestEntityIndex); }
            CS2::C_CSGameRules* dwGameRules() { return MulNX::MRead<CS2::C_CSGameRules*>(this->GetBaseAddress() + cs2_dumper::offsets::client_dll::dwGameRules); }
            float* dwViewMatrix() { return reinterpret_cast<float*>(this->GetBaseAddress() + cs2_dumper::offsets::client_dll::dwViewMatrix); }
            CS2::CCSPlayerController* dwLocalPlayerController() { return MulNX::MRead(reinterpret_cast<CS2::CCSPlayerController**>(this->GetBaseAddress() + cs2_dumper::offsets::client_dll::dwLocalPlayerController)); }
            CS2::C_CSPlayerPawn* dwLocalPlayerPawn() { return MulNX::MRead(reinterpret_cast<CS2::C_CSPlayerPawn**>(this->GetBaseAddress() + cs2_dumper::offsets::client_dll::dwLocalPlayerPawn)); }
            //CS2::C_PlantedC4* dwPlantedC4() { return MulNX::MRead(reinterpret_cast<CS2::C_PlantedC4**>(this->GetBaseAddress() + cs2_dumper::offsets::client_dll::dwPlantedC4)); }


            CS2::C_BaseEntity* GetBaseEntity(int index);
            CS2::C_BaseEntity* GetBaseEntityFromHandle(CS2::CHandleBase handle);
            CS2::C_CSPlayerPawn* GetLocalPlayerPawn();
            CS2::C_CSPlayerPawn* TryGetObservingPawn();
            std::optional<Steam64UID> TryGetObservingSteam64UID();
        };

        class engine2 : public MulNX::Memory::DllModule {
        public:
            using MulNX::Memory::DllModule::DllModule;

            void** ppBuildNumber() { return Schema<void*>(this, cs2_dumper::offsets::engine2_dll::dwBuildNumber); }
            void** ppNetworkGameClient() { return Schema<void*>(this, cs2_dumper::offsets::engine2_dll::dwNetworkGameClient); }
            void** ppClientTickCount() { return Schema<void*>(this, cs2_dumper::offsets::engine2_dll::dwNetworkGameClient_clientTickCount); }
            void** ppDeltaTick() { return Schema<void*>(this, cs2_dumper::offsets::engine2_dll::dwNetworkGameClient_deltaTick); }
            //void** ppIsBackgroundMap() { return Schema<void*>(this, cs2_dumper::offsets::engine2_dll::dwNetworkGameClient_isBackgroundMap); }
            void** ppLocalPlayer() { return Schema<void*>(this, cs2_dumper::offsets::engine2_dll::dwNetworkGameClient_localPlayer); }
            void** ppMaxClients() { return Schema<void*>(this, cs2_dumper::offsets::engine2_dll::dwNetworkGameClient_maxClients); }
            void** ppServerTickCount() { return Schema<void*>(this, cs2_dumper::offsets::engine2_dll::dwNetworkGameClient_serverTickCount); }
            void** ppSignOnState() { return Schema<void*>(this, cs2_dumper::offsets::engine2_dll::dwNetworkGameClient_signOnState); }
            void** ppWindowHeight() { return Schema<void*>(this, cs2_dumper::offsets::engine2_dll::dwWindowHeight); }
            void** ppWindowWidth() { return Schema<void*>(this, cs2_dumper::offsets::engine2_dll::dwWindowWidth); }
        };
    }
}