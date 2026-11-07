#include "HookConsole.hpp"

bool HookConsole::Init() {
    (*this)
        .SubscribeAsyncEx<void>("Game/Command")
        .SubscribeAsyncEx<void>("Game/Command/NoReport")
        ;

    this->SubscribeSync("Hook/LoadLibraryExW/tier0.dll", [this](MulNX::Message& msg) {
        return this->OnTier0Load(msg);
        });

    this->SubscribeSync("Hook/LoadLibraryExW/engine2.dll", [this](MulNX::Message& msg) {
        return this->OnEngine2Load(msg);
        });

    this->SubscribeSync("Hook/CSMainLoop", [this](auto&&...) {
        this->OnMainLoop();
        });

    this->SubscribeSync("System/Init/End", [this](MulNX::Message& msg) {
        auto* pMsgManager = this->FindModule<MulNX::MessageManager>("MessageManager");
        for (auto& [type, meta] : pMsgManager->GetMsgInfo()) {
            if (!meta.isAsync)continue;
            if (!meta.makingHandler)continue;
            const auto& cmdName = meta.RawString;
            auto callbackCCommand = [=](CS2::Console::CCommand* pCmd)->void {
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
        });

    this->SendTask("Update", "CSControl", [this]() {
        this->Update();
        return true;
        });

    return true;
}

HookConsole& HookConsole::RegisterCmd(std::string&& name, std::move_only_function<void(CS2::Console::CCommand*)> callback) {
    this->LogInfo(std::format("请求了指令创建: {}", name));
    MulNXCS2CmdCallback CmdCallback(std::move(callback));
    MulNXCmd cmd(std::move(name), "one MulNX Cmd", std::move(CmdCallback));
    this->CS2Cmds.push_back(std::move(cmd));
    return *this;
}
void HookConsole::OnEngine2Load(MulNX::Message& msg) {
    this->executor = IVClass::Assume(this->CS2->Source2EngineToClient001)->GetVFunc<void(int, const char*, int, double, int64_t)>(51);
}

void HookConsole::OnTier0Load(MulNX::Message& msg) {
    auto VEngineCvar007 = (uintptr_t)this->CS2->tier0.GetProcAddressT<void* (const char*, int*)>("CreateInterface")("VEngineCvar007", nullptr);
    this->pVEngineCvar007 = reinterpret_cast<CS2::Systems::CCvar*>(VEngineCvar007);
    this->pTrySetConVarValueFromString = (TrySetConVarValueFromString_t)this->CS2->tier0.GetTextRegion()
        .FindRegion(CS2::Signatures::Console::TrySetConVarValueFromString).Data();

    this->GetFirstCvarIterator = IVClass::Assume(VEngineCvar007)->GetVFunc<void* (uint64_t&)>(12);
    this->GetNextCvarIterator = IVClass::Assume(VEngineCvar007)->GetVFunc<void* (uint64_t&, uint64_t)>(13);
    this->GetCVarByIndex = IVClass::Assume(VEngineCvar007)->GetVFunc<CS2::Console::ConVar * (uint64_t)>(41);

    this->hkVEngineCvar007_RegisterConCommand = MulNX::Hook::Create((uint8_t*)IVClass::Assume(VEngineCvar007)->GetVFuncPtr(42), [this](MulNX::Hook* hk, RegContext* ctx) {
        hk->ResetCallback([this](MulNX::Hook* hk, RegContext* ctx) {return this->HandleOnRegisterConCommand(hk, ctx);});
        this->HandleOnRegisterConCommand(hk, ctx);

        this->LogInfo("开始注册MulNX的控制台指令");
        for (auto& cmd : this->CS2Cmds) {
            CS2::Console::ConCommand conCommand(
                cmd.name.c_str(),
                cmd.help.c_str(),
                CS2::Console::FCVAR_CLIENTDLL,
                &cmd.callback
            );
            CS2::Systems::CCvar::HConItem handle;
            this->pVEngineCvar007->RegisterConCommand(&handle, &conCommand);
        }
        this->LogSucc(std::format("共注册指令数：{}", this->CS2Cmds.size()));

        return MulNX::Hook::Then::Continue;
        }
    ).value();
    this->RegisterAttachHook(this->hkVEngineCvar007_RegisterConCommand, "VEngineCvar007::RegisterConCommand");
}

void HookConsole::OnMainLoop() {
    std::string cmd;

    if (this->bufferHighPriorityGameCmds.try_dequeue(cmd)) {
        this->executor(0, cmd.c_str(), 1, 0.0, 0LL);
        while (this->bufferHighPriorityGameCmds.try_dequeue(cmd)) {
            this->executor(0, cmd.c_str(), 1, 0.0, 0LL);
        }
        return;
    }
    while (this->bufferGameCmds.try_dequeue(cmd)) {
        auto execByEngine = [&]() {
            this->LogError(std::format("疑似错误的指令：{}", cmd));
            this->executor(0, cmd.c_str(), 1, 0.0, 0LL);
            };

        // 1. 切出命令名，不区分有没有参数
        auto pos = cmd.find(' ');
        char saved = 0;
        if (pos != std::string::npos) {
            saved = cmd[pos];
            cmd[pos] = '\0';          // 临时截断，c_str() 即命令名
        }
        const char* name = cmd.c_str();   // 无空格时是整条 cmd

        // 2. 先查找，再决定怎么执行
        CS2::Systems::CCvar::HConItem h;
        bool isCmd = this->pVEngineCvar007->FindConCommand(&h, name)->IsValid();
        bool isVar = !isCmd && this->pVEngineCvar007->FindConVar(&h, name)->IsValid();

        // 3. 恢复空格，后续需要完整 cmd
        if (pos != std::string::npos) {
            cmd[pos] = saved;
        }

        // 4. ConCommand：有无参数都走 DispatchConCommand
        if (isCmd) {
            CS2::Console::CCommand cmdTokenized{};
            cmdTokenized.Tokenize(cmd.c_str());

            CS2::Systems::CCvar::CCommandContext ctx;
            ctx.eTarget = CS2::Systems::CCvar::CCommandContext::CommandTarget_t::CT_FIRST_SPLITSCREEN_CLIENT;
            ctx.nPlayerSlot.data = 0;

            this->pVEngineCvar007->DispatchConCommand(h, ctx, cmdTokenized);
            continue;
        }

        // 5. ConVar：有参数才设值；无参数是查询，交引擎回显
        if (isVar) {
            if (pos == std::string::npos) {
                return execByEngine();       // 例如 "cl_crosshairstyle" 单独输入
            }

            const char* value = cmd.c_str() + pos + 1;
            CS2::Console::ConVar* cv = this->pVEngineCvar007->GetConVar(h);

            if (cv && this->pTrySetConVarValueFromString) {
                ConVarRef ref{ h, cv };
                if (this->pTrySetConVarValueFromString(&ref, -1, value)) {
                    this->LogInfo(std::format("已分派 ConVar {} = {}", cv->szName, value));
                    continue;
                }
                this->LogWarning(std::format("解析 ConVar {} 值失败: {}", cv->szName, value));
            }

            return execByEngine();
        }

        // 6. 既不是 ConCommand 也不是 ConVar：引擎处理
        return execByEngine();
    }
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
    auto&& [pCmd] = msg.Access<CS2::Console::ConCommand*>();
    pCmd = (CS2::Console::ConCommand*)ctx->r8;
    this->PublishSync(msg);
    return MulNX::Hook::Then::Continue;
}

CS2::Console::ConVar* HookConsole::GetCVarByName(std::string_view var_name)const {
    uint64_t i = 0;
    this->GetFirstCvarIterator(i);
    while (i != 0xFFFFFFFF) {
        CS2::Console::ConVar* pCvar = nullptr;
        pCvar = this->GetCVarByIndex(i);
        if (strcmp(pCvar->szName, var_name.data()) == 0) {
            return pCvar;
        }
        this->GetNextCvarIterator(i, i);
    }
    return nullptr;
}

void HookConsole::UnlockHiddenCVars(int& Count)const {
    uint64_t i = 0;
    this->GetFirstCvarIterator(i);
    while (i != 0xFFFFFFFF) {
        CS2::Console::ConVar* pConVar = this->GetCVarByIndex(i);
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
        CS2::Console::ConVar* pConVar = this->GetCVarByIndex(i);
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