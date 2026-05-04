#pragma once

#include <MulNX/MulNX.hpp>
#include <MulNX/Base/Math/Math.hpp>
#include <MulNX/Base/NewestBuffer/NewestBuffer.hpp>
#include <MulNXExtensions/WinExt/WinExt.hpp>
#include <MulNXExtensions/CS2/Signatures.hpp>

#include <MulNXExtensions/CS2/CSClasses/tree/tree.hpp>
#include <MulNXExtensions/CS2/CSClasses/GlobalVars/GlobalVars.hpp>
#include <MulNXExtensions/CS2/CSClasses/CSDll/CSDll.hpp>
#include <MulNXExtensions/CS2/CSClasses/C_CSGameRules/C_CSGameRules.hpp>

#include "ConVarSystem/ConVarSystem.hpp"
#include "FreeCameraController/FreeCameraController.hpp"
#include "AdvancedViewController/AdvancedViewController.hpp"

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

class Dofs {
public:
    float* pNearBlurry = nullptr;
    float* pNearCrisp = nullptr;
    float* pFarCrisp = nullptr;
    float* pFarBlurry = nullptr;
};

class ControlView {
public:
    std::atomic<bool> hasViewToGame = false;
    MulNX::NewestBuffer<MulNX::Math::View> ViewToGame{};
    MulNX::NewestBuffer<MulNX::Math::View> currentView{};
    std::atomic<float> InputRoll = 0;
    std::atomic<bool> CameraMode = false;
    std::atomic<int> WindowWidth = 1920;
    std::atomic<int> WindowHeight = 1080;
    
    Dofs dofs{};
};

class CSController final :public MulNX::ModuleBase {
private:
    MulNX::TimeBridge timeBridge{ this };
protected:
    D_GameData CS2EBGameData{};
private:
    ControlView controlView{};

    FreeCameraController* pFreeCameraController{};
    AdvancedViewController* pAdvancedViewController = nullptr;

    void* Source2EngineToClient001 = nullptr;
    // 控制台指令执行器
    VExecutor<void(int, const char*, int)> executor{};
    // 控制台变量系统
    C_ConVarSystem CvarSystem{};
    // CS2全局变量
    C_GlobalVars* CSGlobalVars{};
    // 视角控制钩子
    std::unique_ptr<MulNX::Hook> hkPosCallIsPlayingDemo = nullptr;

    bool Window(MulNX::UINode* node);

    void HandleOverrideView(CS2::CViewSetup* viewSetup);
    void HandleCameraSystemPlay(CS2::CViewSetup* viewSetup);

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
    float* GetViewMatrix();
    MulNX::Math::View GetView();
    float GetTime();
    bool JumpTime(const float time);
    float GetWinWidth()const;
    float GetWinHeight()const;
    bool SpecPlayer(int IndexInMap);
    D_Player& GetPlayerMsg(int Index);
    void spec_goto_ex(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& rot);
    void ClearViewOverride();
    void SetDOF(const MulNX::Math::DOFParam& dof);
    void HandleFreeCameraPath(const CameraSystemIO* const IO);
    bool CameraSystemIOOverride(const CameraSystemIO* const IO);
};