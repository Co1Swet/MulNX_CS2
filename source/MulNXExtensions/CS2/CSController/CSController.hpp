#pragma once

#include <MulNX/MulNX.hpp>
#include <MulNX/Base/Math/Math.hpp>
#include <MulNXExtensions/WinExt/WinExt.hpp>
#include <MulNXExtensions/CS2/Signatures.hpp>
#include <MulNXExtensions/CS2/CSClasses/tree/tree.hpp>
#include <MulNXExtensions/CS2/CSClasses/GlobalVars/GlobalVars.hpp>
#include <MulNXExtensions/CS2/CSClasses/CSDll/CSDll.hpp>
#include <MulNXExtensions/CS2/CSClasses/C_CSGameRules/C_CSGameRules.hpp>

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

class CSController final :public MulNX::ModuleBase {
    std::unique_ptr<MulNX::Hook>hkSource2Client002_Init = nullptr;
    std::unique_ptr<MulNX::Hook>hkSource2EngineToClient001_ExecuteCmd = nullptr;
    std::unique_ptr<MulNX::Hook>hkSource2EngineToClient001_IsPlayingDemo = nullptr;
    

    uintptr_t retAddrForShowSpeaker = 0;

    D_GameData CS2EBGameData{};
    // 控制台指令执行器
    VExecutor<void(int, const char*, int)> executor{};
    // CS2全局变量
    C_GlobalVars* CSGlobalVars{};

    std::atomic<int> needToLoadModules = 4;
    MulNX::CoTask currentCoro;
    MulNX::CoTask InitTask();
    void ProcessMsg(MulNX::Message& Msg)override;
    void Main();

    std::set<uintptr_t>detected;
    std::set<uintptr_t>force;

    std::atomic<bool> Source2EngineToClient001ForceReturn = false;
    std::atomic<bool> Source2EngineToClient001ForceReturnValue = true;
    std::atomic<bool> IDemoForceReturn = false;
    std::atomic<bool> IDemoForceReturnValue = true;
    void Window(MulNX::UINode* node);

    void OnClientLoad(MulNX::Message& msg);
    void OnEngine2Load(MulNX::Message& msg);
    void OnTier0Load(MulNX::Message& msg);
    void OnPanoramaLoad(MulNX::Message& msg);
public:
    std::vector<class ICSModule*>ParticipateItCSModules{};

    CS2::Module::Client client{};
    CS2::Module::engine2 engine2{};
    MulNX::Memory::DllModule tier0{};
    MulNX::Memory::DllModule panorama{};
    
    bool Init()override;
    
    VExecutor<void* ()> GetDemo{};
    
    bool SpecPlayer(int IndexInMap);
    D_Player& GetPlayerMsg(int Index);
};