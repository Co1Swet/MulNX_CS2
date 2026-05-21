#pragma once

#include <MulNX/MulNX.hpp>
#include <MulNX/Base/Math/Math.hpp>
#include <MulNXExtensions/WinExt/WinExt.hpp>
#include <MulNXExtensions/CS2/Signatures.hpp>
#include <MulNXExtensions/CS2/CSClasses/tree/tree.hpp>
#include <MulNXExtensions/CS2/CSClasses/GlobalVars/GlobalVars.hpp>
#include <MulNXExtensions/CS2/CSClasses/CSDll/CSDll.hpp>
#include <MulNXExtensions/CS2/CSClasses/C_CSGameRules/C_CSGameRules.hpp>

#include "ConVarSystem/ConVarSystem.hpp"

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
    std::unique_ptr<MulNX::Hook>hkSource2EngineToClient001_ExecuteCmd = nullptr;
    std::unique_ptr<MulNX::Hook>hkSource2EngineToClient001_IsPlayingDemo = nullptr;
    uintptr_t retAddrForShowSpeaker = 0;

    D_GameData CS2EBGameData{};
    void* Source2EngineToClient001 = nullptr;
    // 控制台指令执行器
    VExecutor<void(int, const char*, int)> executor{};
    // 控制台变量系统
    C_ConVarSystem CvarSystem{};
    // CS2全局变量
    C_GlobalVars* CSGlobalVars{};

    std::atomic<int> needToLoadModules = 4;
    MulNX::CoTask currentCoro;
    MulNX::CoTask InitTask();
    void ProcessMsg(MulNX::Message& Msg)override;
    void Main();

    std::mutex ForceMutex;

    std::set<uintptr_t>detected;
    std::set<uintptr_t>force;

    std::atomic<bool> Source2EngineToClient001ForceReturn = false;
    std::atomic<bool> Source2EngineToClient001ForceReturnValue = true;
    std::atomic<bool> IDemoForceReturn = false;
    std::atomic<bool> IDemoForceReturnValue = true;
    void Window(MulNX::UINode* node);
public:
    std::vector<std::function<bool(CS2::CCSPlayerController*, CS2::C_CSPlayerPawn*)>>handlesControlPlayer{};
    
    CS2::Module::Client client{};
    CS2::Module::engine2 engine2{};
    MulNX::Memory::DllModule tier0{};
    MulNX::Memory::DllModule panorama{};
    // 获取控制台变量系统
    C_ConVarSystem& GetCvarSystem() { return this->CvarSystem; }
    
    bool Init()override;
    
    VExecutor<void* ()> GetDemo{};
    
    bool SpecPlayer(int IndexInMap);
    D_Player& GetPlayerMsg(int Index);
};