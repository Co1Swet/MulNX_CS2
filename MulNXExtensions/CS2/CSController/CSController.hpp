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

class CSController;

namespace MulNX {
    class TimeBridge {
        // 指向抽象层的指针，用于调用抽象层的时间函数
        CSController* pCS2;
        //是否在虚拟时间轴播放（偏移时间轴播放）
        bool virtualTimePlaying = false;
        // MulNX时间参考点
        std::chrono::steady_clock::time_point startTime;
        // 用于计算虚拟时间的缓冲变量
        float refreshTime = 0.0f;
        // 比例，用于控制虚拟时间的流速，默认为1.0f（与真实时间相同）
        float scale = 1.0f;
        // 上一次获取的真实时间，用于检测时间回跳等异常情况
        float lastRealTime = 0.0f;
        // 内部更新函数
        void update();
    public:
        TimeBridge() = delete;
        TimeBridge(CSController* pCS2);

        bool RefreshVirtual(bool virtualTimePlaying, float scale);
        float GetReal();
        bool JumpReal(float time);
        bool JumpRealRel(float time);
        float GetVirtual();
        float Get();
    };
}

class CSController final :public MulNX::ModuleBase {
private:
    MulNX::TimeBridge timeBridge{ this };
protected:
    D_GameData CS2EBGameData{};
private:
    void* Source2EngineToClient001 = nullptr;
    // 控制台指令执行器
    VExecutor<void(int, const char*, int)> executor{};
    // 控制台变量系统
    C_ConVarSystem CvarSystem{};
    // CS2全局变量
    C_GlobalVars* CSGlobalVars{};

    bool Window(MulNX::UINode* node);

    void EnlistExecutors();
    void ProcessMsg(MulNX::Message& Msg)override;
    void Update();
public:
    std::vector<std::function<bool(CS2::CCSPlayerController*, CS2::C_CSPlayerPawn*)>>handlesControlPlayer{};
    
    CS2::Module::Client client{};
    CS2::Module::engine2 engine2{};
    MulNX::Memory::DllModule tier0{};
    // 获取控制台变量系统
    C_ConVarSystem& GetCvarSystem() { return this->CvarSystem; }
    
    bool Init()override;
    
    VExecutor<void* ()> GetDemo{};
    VExecutor<int()>GetDemoTick{};
    VExecutor<bool()>IsPlayingDemo{};
    VExecutor<bool()>IsDemoPaused{};
    
    // 返回时间源，由实现创建独占指针，这里返回原始指针
    MulNX::TimeBridge* Time();
    float GetTime();
    bool JumpTime(const float time);
    bool SpecPlayer(int IndexInMap);
    D_Player& GetPlayerMsg(int Index);
};