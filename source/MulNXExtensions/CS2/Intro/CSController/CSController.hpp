#pragma once
#include <MulNX/MulNX.hpp>
#include <MulNX/Base/Math/Math.hpp>
#include <MulNXUtils/WinExt/WinExt.hpp>
#include <Intro/Signatures.hpp>
#include <Intro/CSClasses/tree/tree.hpp>
#include <Intro/CSClasses/GlobalVars/GlobalVars.hpp>
#include <Intro/CSClasses/CSDll/CSDll.hpp>
#include <Intro/CSClasses/C_CSGameRules/C_CSGameRules.hpp>
#include <MulNXUtils/MemInsights/RetEditor/RetEditor.hpp>

//1到10为玩家，0为本地
class D_Player {
public:
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT3 EyePosition;
    DirectX::XMFLOAT3 Rotation;
    int HP;
    int Team;
    bool Alive;
    int IndexInEntityList;
    int IndexInMap;
};

class D_GameData {
public:
    D_Player Players[11];

};

class CSController final :public MulNX::Module<CSController> {
    std::unique_ptr<MulNX::Hook>hkSource2Client002_Init = nullptr;
    std::unique_ptr<MulNX::Hook>hkSource2EngineToClient001_ExecuteCmd = nullptr;
    std::unique_ptr<MulNX::Hook>hkSource2EngineToClient001_IsPlayingDemo = nullptr;

    uintptr_t retAddrForShowSpeaker = 0;
    std::atomic<int> needToLoadModules = 4;
    MulNX::CoTask InitTask();
    void Main();

    RetEditor checkSource2EngineToClient001_IsPlayingDemo{};
    std::atomic<bool> Source2EngineToClient001ForceReturn = false;
    std::atomic<bool> Source2EngineToClient001ForceReturnValue = true;

    void Window();

    void OnClientLoad(MulNX::Message& msg);
    void OnEngine2Load(MulNX::Message& msg);
    void OnTier0Load(MulNX::Message& msg);
    void OnPanoramaLoad(MulNX::Message& msg);
public:
    // CS2全局变量
    C_GlobalVars* CSGlobalVars{};
    std::vector<class ICSModule*>ParticipateItCSModules{};

    CS2::Module::Client client{};
    CS2::Module::engine2 engine2{};
    void* Source2EngineToClient001 = nullptr;
    MulNX::Memory::DllModule tier0{};
    MulNX::Memory::DllModule panorama{};
    
    bool Init()override;
    
    VExecutor<void* ()> GetDemo{};
    
    D_GameData CS2EBGameData{};
    D_Player& GetPlayerMsg(int Index);
};