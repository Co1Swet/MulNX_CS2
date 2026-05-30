#include "CSController.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXThirdParty/All_cs2_dumper.hpp>

void CSController::Window(MulNX::UINode* node) {
    // auto w = MulNX::UI::RAIIWindow("实验性功能", this->showWindow);
    // if (!w)return;
    // MulNX::UI::Checkbox("Source2EngineToClient001 强制返回？", this->Source2EngineToClient001ForceReturn);
    // MulNX::UI::Checkbox("Source2EngineToClient001 返回值", this->Source2EngineToClient001ForceReturnValue);

    // MulNX::UI::Checkbox("IDemo 强制返回？", this->IDemoForceReturn);
    // MulNX::UI::Checkbox("IDemo 返回值", this->IDemoForceReturnValue);

    // std::unique_lock lock(this->ForceMutex);
    // ImGui::SeparatorText("检测到的调用点");
    // if (this->detected.empty()) {
    //     ImGui::TextDisabled("（空）");
    // }
    // else {
    //     for (const auto& call : this->detected) {
    //         ImGui::Text("%llX", call);                      // 十六进制显示
    //         ImGui::SameLine();
    //         bool alreadyForced = (this->force.find(call) != this->force.end());
    //         if (alreadyForced) {
    //             ImGui::TextDisabled("已添加");
    //         }
    //         else {
    //             // 使用 call 作为 ID 后缀，保证按钮唯一
    //             if (ImGui::Button(("添加##" + std::to_string(call)).c_str())) {
    //                 this->force.insert(call);
    //             }
    //         }
    //     }
    // }

    // ImGui::SeparatorText("已强制返回的调用点");
    // if (this->force.empty()) {
    //     ImGui::TextDisabled("（空）");
    // }
    // else {
    //     std::vector<uintptr_t> toRemove;
    //     for (const auto& call : this->force) {
    //         ImGui::Text("%llX", call);
    //         ImGui::SameLine();
    //         if (ImGui::Button(("移除##" + std::to_string(call)).c_str())) {
    //             toRemove.push_back(call);
    //         }
    //     }
    //     for (auto addr : toRemove) {
    //         this->force.erase(addr);
    //     }
    // }
}

bool CSController::Init() {
    this->CS2Cmds.reserve(100);
    this->showWindow = true;
    this->ISys()
        .SubscribeAsync("Demo/GotoTick")
        .SubscribeAsync("Game/Command")
        .SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {return this->OnClientLoad(msg);})
        .SubscribeSync("Hook/LoadLibraryExW/engine2.dll", [this](MulNX::Message& msg) {return this->OnEngine2Load(msg);})
        .SubscribeSync("Hook/LoadLibraryExW/tier0.dll", [this](MulNX::Message& msg) {return this->OnTier0Load(msg);})
        .SubscribeSync("Hook/LoadLibraryExW/panorama.dll", [this](MulNX::Message& msg) {return this->OnPanoramaLoad(msg);})
        .SubscribeSync("Hook/RegisterConCommand/RegisterOurCmd",
            [this](MulNX::Message& msg) {
                this->RegisterCS2Cmd("mulnx_record_start", "this is MulNX Cmd", [this](CCommand* a) {
                    return;
                    });
                this->RegisterCS2Cmd("mulnx_record_end", "this is MulNX Cmd", [this](CCommand* a) {
                    return;
                    });
            });
        ;

    this->currentCoro = InitTask();
    this->currentCoro.resume();

    this->ISys().SendTask("Update", "CSControl", [this]()->bool {
        this->Update();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        return true;
        });
    

    return true;
}
MulNX::CoTask CSController::InitTask() {
    // 等待必要模块加载完成
    co_await this->WaitUntil([this]()->bool {return this->needToLoadModules.load() == 0;});

    this->ISys().SendTask("Main", "CSControl", [this]()->bool {
        try {
            this->Main();
        }
        catch (const std::runtime_error& e) {
            this->ISys().LogWarning("在更新数据时捕获到异常：" + std::string(e.what()));
        }
        return true;
        });

    // this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {return this->Window(node);});

    co_return;
}

void CSController::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Game/Command"_hash: {
        auto cmd = std::move(msg.asp.get<MulNX::NetExt>()->str1);
        this->executor(0, cmd.c_str(), 1);
        break;
    }
    case "Demo/GotoTick"_hash: {
        int tick = msg.p1.low<int>();
        auto cmd = std::format("demo_gototick {}", tick);
        this->executor(0, cmd.c_str(), 1);
        break;
    }
    }
}

void CSController::OnClientLoad(MulNX::Message& msg) {
    this->client = CS2::Module::Client(L"client.dll");
    void* pClient = this->client.GetProcAddressT<void* (const char*, int*)>("CreateInterface")("Source2Client002", nullptr);
    this->hkSource2Client002_Init = MulNX::Hook::Create((uint8_t*)IVClass::Assume(pClient)->GetVFuncPtr(3), [this](MulNX::Hook* hk, RegContext* ctx) {
        hk->CallMaybeOrigin(0, ctx);
        this->ISys().PublishSync("Hook/Source2Client002::Inited"_hash);
        this->hkSource2Client002_Init->Detach();
        return MulNX::Hook::Then::Return;
        }
    ).value();
    this->hkSource2Client002_Init->Attach();
    this->ISys().LogSucc(I18n("hook.attached", "Source2Client002::Init"));

    auto back = this->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::ifShowSpeaker).Rdata();
    this->retAddrForShowSpeaker = reinterpret_cast<uintptr_t>(back) - 4;
    --this->needToLoadModules;
}
void CSController::OnEngine2Load(MulNX::Message& msg) {
    this->engine2 = CS2::Module::engine2(L"engine2.dll");
    auto Source2EngineToClient001 = this->engine2.GetProcAddressT<void* (const char*, int*)>("CreateInterface")("Source2EngineToClient001", nullptr);
    // demo
    this->GetDemo = IVClass::Assume(Source2EngineToClient001)->GetVFunc<void* ()>(68);
    // cmd
    this->executor = IVClass::Assume(Source2EngineToClient001)->GetVFunc<void(int, const char*, int)>(50);
    this->hkSource2EngineToClient001_ExecuteCmd = MulNX::Hook::Create((uint8_t*)this->executor.GetRawFuncPtr(), [](MulNX::Hook* hk, RegContext* ctx) {
        static std::mutex mtx;
        std::lock_guard lock(mtx);
        hk->CallMaybeOrigin(0, ctx);
        return MulNX::Hook::Then::Return;
        }).value();
    this->hkSource2EngineToClient001_ExecuteCmd->Attach();
    this->ISys().LogSucc(I18n("hook.attached", "Source2EngineToClient001::ExecuteCmd"));

    // for show speaker
    this->hkSource2EngineToClient001_IsPlayingDemo = MulNX::Hook::Create((uint8_t*)IVClass::Assume(Source2EngineToClient001)->GetVFuncPtr(42), [this](MulNX::Hook* hk, RegContext* ctx) {
        auto returnAddress = *(uintptr_t*)hk->GetRawStackAddr(ctx);
        using Source2EngineToClient001_IsPlayingDemo_t = bool(*)(void*);
        *(bool*)(&ctx->rax) = reinterpret_cast<Source2EngineToClient001_IsPlayingDemo_t>(hk->pMaybeRawFunc)(reinterpret_cast<void*>(ctx->rcx));
        if (returnAddress == this->retAddrForShowSpeaker) {
            *(bool*)(&ctx->rax) = false;
        }
        // std::unique_lock lock(this->ForceMutex);
        // this->detected.insert(callPos);
        // if (this->force.find(callPos) != this->force.end()) {
        //     if (this->Source2EngineToClient001ForceReturn) {
        //         *(bool*)(&ctx->rax) = this->Source2EngineToClient001ForceReturnValue;
        //     }
        // }

        return MulNX::Hook::Then::Return;
        }).value();
    this->hkSource2EngineToClient001_IsPlayingDemo->Attach();
    this->ISys().LogSucc(I18n("hook.attached", "Source2EngineToClient001::IsPlayingDemo"));

    --this->needToLoadModules;
}
void CSController::OnTier0Load(MulNX::Message& msg) {
    this->tier0 = MulNX::Memory::DllModule(L"tier0.dll");
    // 获取CvarSystem
    auto VEngineCvar007 = (uintptr_t)this->tier0.GetProcAddressT<void* (const char*, int*)>("CreateInterface")("VEngineCvar007", nullptr);
    this->CvarSystem.Load(VEngineCvar007);

    this->hkVEngineCvar007_RegisterConCommand = MulNX::Hook::Create((uint8_t*)IVClass::Assume(VEngineCvar007)->GetVFuncPtr(44), [this](MulNX::Hook* hk, RegContext* ctx) {
        auto pOrigCmd = ctx->r8;

        this->RegisterCS2Cmd = [this, &hk, &ctx](std::string&& name, std::string&& help, std::function<void(CCommand*)>&& callback)->void {
            this->CS2Cmds.push_back({ std::move(name), std::move(help), std::move(callback) });
            auto& cmd = this->CS2Cmds.back();

            CCmd cmdForCS2(
                cmd.name.c_str(),
                cmd.help.c_str(),
                FCVAR_CLIENTDLL,
                &cmd.callback
            );
            ctx->r8 = (uint64_t)&cmdForCS2;
            hk->CallMaybeOrigin(5, ctx);

            };

        this->ISys().PublishSync("Hook/RegisterConCommand/RegisterOurCmd"_hash);

        ctx->r8 = pOrigCmd;
        hk->CallMaybeOrigin(5, ctx);
        hk->ResetCallback([this](MulNX::Hook* hk, RegContext* ctx) {return this->HandleOnRegisterConCommand(hk, ctx);});
        return MulNX::Hook::Then::Return;
        }
    ).value();
    this->hkVEngineCvar007_RegisterConCommand->Attach();
    this->ISys().LogSucc(I18n("hook.attached", "VEngineCvar007::RegisterConCommand"));
    --this->needToLoadModules;
}
void CSController::OnPanoramaLoad(MulNX::Message& msg) {
    this->panorama = MulNX::Memory::DllModule(L"panorama.dll");
    --this->needToLoadModules;
}

void CSController::Main() {
    // 获取CS2全局变量
    this->CSGlobalVars = MulNX::MRead<C_GlobalVars*>(this->client.GetBaseAddress() + cs2_dumper::offsets::client_dll::dwGlobalVars);

    auto pGameRules = this->client.dwGameRules();
    if (!pGameRules) return;

    static int32_t OldRoundStartCount = 0;
    auto currentStartCount = MulNX::MRead(pGameRules->m_nRoundStartCount());
    if (OldRoundStartCount != currentStartCount) {
        this->ISys().PublishAsync("Game/NewRound"_hash);
        OldRoundStartCount = currentStartCount;
    }

    std::unique_lock lock(this->smutex);
    // 玩家控制器，地图上从1到10
    int playerNum = 0;
    for (int i = 0; i < this->client.dwGameEntitySystem_highestEntityIndex(); ++i) {
        auto* entity = this->client.GetBaseEntity(i);
        if (!entity)continue;

        auto* controller = entity->As<CS2::CCSPlayerController>();
        auto hPawn = MulNX::MRead(controller->m_hPlayerPawn());
        auto* pawn = this->client.GetBaseEntityFromHandle(hPawn.GetIndexInEntityList())->As<CS2::C_CSPlayerPawn>();
        if (!pawn)continue;

        auto team = MulNX::MRead(pawn->iTeamNum());
        if (team != CS2::ui8TeamNum::T && team != CS2::ui8TeamNum::CT)continue;
        ++playerNum;

        for (const auto& handle : this->handlesControlPlayer) {
            handle(controller, pawn);
        }
        //MulNX::MWrite(pawn->m_entitySpottedState()->m_bSpotted(), true);

        if (playerNum <= 10) {
            auto& CS2EBEntity = this->CS2EBGameData.Players[playerNum];
            CS2EBEntity.Position = MulNX::MRead(pawn->vOldOrigin());
            CS2EBEntity.EyePosition = MulNX::MRead(pawn->vOldOrigin()) + MulNX::MRead(pawn->vecViewOffset());
            CS2EBEntity.Rotation = MulNX::MRead(pawn->angEyeAngles());
            CS2EBEntity.HP = MulNX::MRead(pawn->iHealth());
            CS2EBEntity.Team = static_cast<int>(team);
            CS2EBEntity.Alive = CS2EBEntity.HP;
            CS2EBEntity.IndexInMap = playerNum;
        }


    }
    return;
}

MulNX::Hook::Then CSController::HandleOnRegisterConCommand(MulNX::Hook* hk, RegContext* ctx) {
    CCmd* pCmd = (CCmd*)ctx->r8;
    std::string_view name = pCmd->m_pszName;
    if (name == "playdemo") {
        auto pf = pCmd->m_pCommandCallback;
        this->hkPlaydemo = MulNX::Hook::Create((uint8_t*)pCmd->m_pCommandCallback, [this](MulNX::Hook* hk, RegContext* ctx) {
            auto cmd = (CCommand*)ctx->rdx;
            auto str = std::string(cmd->pRawString);
            this->ISys().LogInfo(str);
            return MulNX::Hook::Then::Continue;
            }).value();
        this->hkPlaydemo->Attach();

        return MulNX::Hook::Then::Continue;
    }


    return MulNX::Hook::Then::Continue;
}

// C_ConVar* m_yawPtr = nullptr;
// m_yawPtr = this->CvarSystem.GetCvar("m_yaw");
// std::vector<int> schemas;
// for (int schema = 0;schema < 0x100;++schema) {
//     const float targetValue = 0.0165f;
//     bool valueIsRight = false;
//     auto checkValue = [&]()->bool {
//         float currentValue = *(float*)((uintptr_t)m_yawPtr + schema);
//         if (fabs(currentValue - targetValue) < 0.001f) {
//             valueIsRight = true;
//         }
//         return true;// 无崩溃
//         };
//     MulNX::Base::UnsafeFunc(checkValue);
//     if (valueIsRight) {
//         schemas.push_back(schema);
//     }
// }

bool CSController::SpecPlayer(int IndexInMap) {
    this->ISys().AsyncCommand("spec_mode 2;spec_player " + std::to_string(this->CS2EBGameData.Players[IndexInMap].IndexInMap));
    return true;
}
D_Player& CSController::GetPlayerMsg(int Index) {
    //std::shared_lock lock(this->GetMtx());
    return this->CS2EBGameData.Players[Index];
}