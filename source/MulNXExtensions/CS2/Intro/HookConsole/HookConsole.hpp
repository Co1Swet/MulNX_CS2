#pragma once
#include <Game/Systems/ICvar.hpp>
#include <Intro/CSModuleBase.hpp>

class HookConsole final :public CSModuleBase {
    CS2::Systems::CCvar* pVEngineCvar007 = nullptr;

    struct ConVarRef {
        CS2::Systems::CCvar::HConItem hConVar;   // 0x00
        CS2::Console::ConVar* pConVar;  // 0x08
    };
    static_assert(sizeof(ConVarRef) == 0x10);
    using TrySetConVarValueFromString_t = bool(*)(ConVarRef* ref, int slot, const char* value);
    TrySetConVarValueFromString_t pTrySetConVarValueFromString = nullptr;

    class MulNXCmd {
    public:
        std::string name;
        std::string help;
        MulNXCS2CmdCallback callback;
    };
    std::vector<MulNXCmd> CS2Cmds{};

    std::unique_ptr<MulNX::Hook>hkVEngineCvar007_RegisterConCommand = nullptr;
    
    // 控制台指令执行器
    VExecutor<void(int, const char*, int, double, int64_t)> executor{};
    
    MulNX::Hook::Then HandleOnRegisterConCommand(MulNX::Hook* hk, RegContext* ctx);
    void OnTier0Load(MulNX::Message& msg);
    void OnEngine2Load(MulNX::Message& msg);
    void OnMainLoop();

    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;
public:
    moodycamel::ConcurrentQueue<std::string>bufferGameCmds;
    
    HookConsole& RegisterCmd(std::string&& name, std::move_only_function<void(CS2::Console::CCommand*)> callback);
    // 得到第一个Cvar的迭代器
    VExecutor<void* (uint64_t&)>GetFirstCvarIterator{};
    // 得到下一个Cvar的迭代器
    VExecutor<void* (uint64_t&, uint64_t)>GetNextCvarIterator{};
    // 通过迭代器ID获取Cvar
    VExecutor<CS2::Console::ConVar* (uint64_t)>GetCVarByIndex{};
    // 通过名称获取Cvar
    CS2::Console::ConVar* GetCVarByName(std::string_view var_name)const;

    void UnlockHiddenCVars(int& Count)const;
    void LockAllCvars(int& Count)const;
};