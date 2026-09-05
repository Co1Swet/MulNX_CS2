#pragma once
#include <MulNX/MulNX.hpp>
#include <MulNXUtils/WinExt/WinExt.hpp>
#include <Intro/Signatures.hpp>
#include <Game/Games.hpp>
#include <MulNXUtils/MemInsights/RetEditor/RetEditor.hpp>
#include <MulNXUtils/WinExt/HookMixin.hpp>

class CSController final :public MulNX::Module<CSController>, public HookMixin<CSController> {
    std::unique_ptr<MulNX::Hook>hkSource2Client002_Init = nullptr;
    std::unique_ptr<MulNX::Hook>hkSource2EngineToClient001_ExecuteCmd = nullptr;
    std::unique_ptr<MulNX::Hook>hkSource2EngineToClient001_IsPlayingDemo = nullptr;

    RetEditor checkSource2EngineToClient001_IsPlayingDemo{};
    std::atomic<bool> Source2EngineToClient001ForceReturn = false;
    std::atomic<bool> Source2EngineToClient001ForceReturnValue = true;

    void Window();

    void OnClientLoad(MulNX::Message& msg);
    void OnEngine2Load(MulNX::Message& msg);
    void OnTier0Load(MulNX::Message& msg);
    void OnPanoramaLoad(MulNX::Message& msg);

    bool Init()override;
public:
    CS2::Module::Client client{};
    CS2::Module::engine2 engine2{};
    void* Source2EngineToClient001 = nullptr;
    MulNX::Memory::DllModule tier0{};
    MulNX::Memory::DllModule panorama{};
    
    VExecutor<void* ()> GetDemo{};

    // CS2全局变量
    CCSGlobalVars* CSGlobalVars{};
};