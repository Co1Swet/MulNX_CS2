#pragma once
#include <MulNXExtensions/CS2/CSModuleBase.hpp>
#include <MulNXExtensions/CS2/CSClasses/Consoles.hpp>

class HookConsole final :public CSModuleBase {
    friend class ConsoleManager;
    class MulNXCmd {
    public:
        std::string name;
        std::string help;
        MulNXCS2CmdCallback callback;
    };

    std::unique_ptr<MulNX::Hook>hkVEngineCvar007_RegisterConCommand = nullptr;
    std::unique_ptr<MulNX::Hook>hkPlaydemo = nullptr;

    // 控制台指令执行器
    VExecutor<void(int, const char*, int)> executor{};

    std::vector<MulNXCmd> CS2Cmds{};
    MulNX::Hook::Then HandleOnRegisterConCommand(MulNX::Hook* hk, RegContext* ctx);
    void OnTier0Load(MulNX::Message& msg);

    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;
public:
    //得到第一个Cvar的迭代器
    VExecutor<void* (uint64_t&)>GetFirstCvarIterator{};
    //得到下一个Cvar的迭代器
    VExecutor<void* (uint64_t&, uint64_t)>GetNextCvarIterator{};
    //通过迭代器ID获取Cvar
    VExecutor<C_ConVar* (uint64_t)>GetCVarByIndex{};
    //通过名称获取Cvar
    C_ConVar* GetCVarByName(const char* var_name)const;

    void UnlockHiddenCVars(int& Count)const;
    void LockAllCvars(int& Count)const;

    //通过名称获取Cvar，使用缓存加速
    C_ConVar* GetCvar(const std::string& CvarName);
    //注册CS2控制台命令
    std::function<void(std::string&&, std::string&&, std::function<void(CCommand*)>&&)>RegisterCS2Cmd = nullptr;
};