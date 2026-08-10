#include "HookConsole.hpp"


bool HookConsole::Init() {
    this->CS2Cmds.reserve(100);
    (*this)
        .SubscribeSync("Hook/LoadLibraryExW/tier0.dll", [this](MulNX::Message& msg) {return this->OnTier0Load(msg);})
        .SubscribeSync("Hook/LoadLibraryExW/engine2.dll", [this](MulNX::Message& msg) {return this->OnEngine2Load(msg);})
        .SubscribeAsync("Game/Command")
        .SubscribeAsync("Game/Command/NoReport")
        ;

    this->SendTask("Update", "CSControl", [this]() {
        this->Update();
        return true;
        });

    this->SubscribeSync("System/Init/End", [this](MulNX::Message& msg) {
        auto* pMsgManager = this->FindModule<MulNX::MessageManager>("MessageManager");
        for (auto& [type, meta] : pMsgManager->GetMsgInfo()) {
            if (!meta.isAsync)continue;
            if (!meta.makingHandler)continue;
            const auto& cmdName = meta.RawString;
            auto callbackCCommand = [=](CCommand* pCmd)->void {
                std::string_view raw(pCmd->pRawString);
                raw = raw.substr(raw.find(' ') + 1);
                MulNX::Message msg(type);
                auto makingHandler = meta.makingHandler;
                try {
                    makingHandler(msg, raw);
                }
                catch (const std::exception& e) {
                    this->LogError(std::format("转换CS2控制台命令 '{}' 时发生异常: {}", pCmd->pRawString, e.what()));
                    return;
                }

                this->PublishAsync(std::move(msg));
                };

            MulNXCS2CmdCallback CmdCallback(std::move(callbackCCommand));
            auto full = "MulNX/" + meta.RawString;
            MulNXCmd cmd(std::move(full), "one MulNX Cmd", std::move(CmdCallback));
            this->CS2Cmds.push_back(std::move(cmd));
        }

        return;
        });

    return true;
}

HookConsole& HookConsole::RegisterCmd(std::string&& name, std::function<void(CCommand*)>&& callback) {
    this->LogInfo(std::format("请求了指令创建: {}", name));
    MulNXCS2CmdCallback CmdCallback(std::move(callback));
    MulNXCmd cmd(std::move(name), "one MulNX Cmd", std::move(CmdCallback));
    this->CS2Cmds.push_back(std::move(cmd));
    return *this;
}
void HookConsole::OnEngine2Load(MulNX::Message& msg) {
    this->executor = IVClass::Assume(this->CS2->Source2EngineToClient001)->GetVFunc<void(int, const char*, int, double, int64_t)>(51);
    auto Pos_Call_CInputService_ProcessCommands = this->CS2->engine2.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Utils::Pos_Call_CInputService_ProcessCommands);
    this->hkPos_Call_CInputService_ProcessCommands = MulNX::Hook::Create(Pos_Call_CInputService_ProcessCommands.Data(), [this](MulNX::Hook* hk, RegContext* ctx) {
        if (!this->pGlobalVars->SystemReady.load(std::memory_order_relaxed))return MulNX::Hook::Then::Continue;
        std::string cmd;
        if (this->bufferGameCmds.try_dequeue(cmd))
            this->executor(0, cmd.c_str(), 1, 0.0, 0LL);
        return MulNX::Hook::Then::Continue;
        }, true).value();
    this->RegisterAttachHook(this->hkPos_Call_CInputService_ProcessCommands, "Pos_Call_CInputService_ProcessCommands");
}

void HookConsole::OnTier0Load(MulNX::Message& msg) {
    auto VEngineCvar007 = (uintptr_t)this->CS2->tier0.GetProcAddressT<void* (const char*, int*)>("CreateInterface")("VEngineCvar007", nullptr);

    this->GetFirstCvarIterator = IVClass::Assume(VEngineCvar007)->GetVFunc<void* (uint64_t&)>(12);
    this->GetNextCvarIterator = IVClass::Assume(VEngineCvar007)->GetVFunc<void* (uint64_t&, uint64_t)>(13);
    this->GetCVarByIndex = IVClass::Assume(VEngineCvar007)->GetVFunc<C_ConVar * (uint64_t)>(41);

    this->hkVEngineCvar007_RegisterConCommand = MulNX::Hook::Create((uint8_t*)IVClass::Assume(VEngineCvar007)->GetVFuncPtr(42), [this](MulNX::Hook* hk, RegContext* ctx) {
        hk->ResetCallback([this](MulNX::Hook* hk, RegContext* ctx) {return this->HandleOnRegisterConCommand(hk, ctx);});
        auto pOrigCmd = ctx->r8;

        this->LogInfo("开始注册MulNX的控制台指令");
        for (auto& cmd : this->CS2Cmds) {
            CCmd cmdForCS2(
                cmd.name.c_str(),
                cmd.help.c_str(),
                FCVAR_CLIENTDLL,
                &cmd.callback
            );
            ctx->r8 = (uint64_t)&cmdForCS2;
            hk->CallMaybeOrigin(5, ctx);
        }
        this->LogSucc(std::format("共注册指令数：{}", this->CS2Cmds.size()));

        ctx->r8 = pOrigCmd;
        hk->CallMaybeOrigin(5, ctx);

        return MulNX::Hook::Then::Return;
        }
    ).value();
    this->RegisterAttachHook(this->hkVEngineCvar007_RegisterConCommand, "VEngineCvar007::RegisterConCommand");
}

void HookConsole::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Game/Command"_hash: {
        auto cmd = std::move(msg.asp.get<MulNX::NetExt>()->str1);
        this->LogInfo(std::format("已推入MulNX缓冲队列：{}", cmd));
        this->bufferGameCmds.enqueue(std::move(cmd));
        break;
    }
    case "Game/Command/NoReport"_hash: {
        auto cmd = std::move(msg.asp.get<MulNX::NetExt>()->str1);
        this->bufferGameCmds.enqueue(std::move(cmd));
        break;
    }
    }
}

MulNX::Hook::Then HookConsole::HandleOnRegisterConCommand(MulNX::Hook* hk, RegContext* ctx) {
    MulNX::Message msg("Hook/RegisterConCommand"_hash);
    auto&& [pCmd] = msg.Access<CCmd*>();
    pCmd = (CCmd*)ctx->r8;
    this->PublishSync(msg);
    return MulNX::Hook::Then::Continue;
}

C_ConVar* HookConsole::GetCVarByName(const char* var_name)const {
    uint64_t i = 0;
    this->GetFirstCvarIterator(i);
    while (i != 0xFFFFFFFF) {
        C_ConVar* pCvar = nullptr;
        pCvar = this->GetCVarByIndex(i);
        if (strcmp(pCvar->szName, var_name) == 0) {
            return pCvar;
        }
        this->GetNextCvarIterator(i, i);
    }
    return nullptr;
}
C_ConVar* HookConsole::GetCvar(const std::string& CvarName) {
    C_ConVar* pCvar = this->GetCVarByName(CvarName.c_str());
    return pCvar;
}

void HookConsole::UnlockHiddenCVars(int& Count)const {
    uint64_t i = 0;
    this->GetFirstCvarIterator(i);
    while (i != 0xFFFFFFFF) {
        C_ConVar* pConVar = this->GetCVarByIndex(i);
        if (pConVar) {
            if (pConVar->IsHidden()) {
                pConVar->Unhide();
                ++Count;
            }
        }
        this->GetNextCvarIterator(i, i);
    }
    return;
}
void HookConsole::LockAllCvars(int& Count)const {
    uint64_t i = 0;
    this->GetFirstCvarIterator(i);
    while (i != 0xFFFFFFFF) {
        C_ConVar* pConVar = this->GetCVarByIndex(i);
        if (pConVar) {
            if (!pConVar->IsHidden()) {
                pConVar->Hide();
                ++Count;
            }
        }
        this->GetNextCvarIterator(i, i);
    }
    return;
}