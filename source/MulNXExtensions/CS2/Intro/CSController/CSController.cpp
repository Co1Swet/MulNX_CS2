#include "CSController.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <Intro/CSModuleBase.hpp>
#include <MulNXThirdParty/All_cs2_dumper.hpp>

void CSController::Window(MulNX::UINode* node) {
    auto w = MulNX::UI::RAIIWindow("实验性功能");
    MulNX::UI::Checkbox("Source2EngineToClient001 强制返回？", this->Source2EngineToClient001ForceReturn);
    MulNX::UI::Checkbox("Source2EngineToClient001 返回值", this->Source2EngineToClient001ForceReturnValue);
    this->checkSource2EngineToClient001_IsPlayingDemo.Render();
}

bool CSController::Init() {
    (*this)
        .SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {return this->OnClientLoad(msg);})
        .SubscribeSync("Hook/LoadLibraryExW/engine2.dll", [this](MulNX::Message& msg) {return this->OnEngine2Load(msg);})
        .SubscribeSync("Hook/LoadLibraryExW/tier0.dll", [this](MulNX::Message& msg) {return this->OnTier0Load(msg);})
        .SubscribeSync("Hook/LoadLibraryExW/panorama.dll", [this](MulNX::Message& msg) {return this->OnPanoramaLoad(msg);})
        ;

    this->InitTask().resume();

    this->SendTask("Update", "CSControl", [this]()->bool {
        this->Update();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        return true;
        });
    
    // this->SendUIRoot("RTEST", [this](MulNX::UINode* node) {return this->Window(node);});
    
    return true;
}
MulNX::CoTask CSController::InitTask() {
    // 等待必要模块加载完成
    co_await this->WaitUntil([this]()->bool {return this->needToLoadModules.load() == 0;});

    this->SendTask("Main", "CSControl", [this]()->bool {
        try {
            this->Main();
        }
        catch (const std::runtime_error& e) {
            this->LogWarning("在更新数据时捕获到异常：" + std::string(e.what()));
        }
        return true;
        });

    co_return;
}

void CSController::OnClientLoad(MulNX::Message& msg) {
    this->client = CS2::Module::Client(L"client.dll");
    void* pClient = this->client.GetProcAddressT<void* (const char*, int*)>("CreateInterface")("Source2Client002", nullptr);
    this->hkSource2Client002_Init = MulNX::Hook::Create((uint8_t*)IVClass::Assume(pClient)->GetVFuncPtr(3), [this](MulNX::Hook* hk, RegContext* ctx) {
        hk->CallMaybeOrigin(0, ctx);
        this->PublishSync("Hook/Source2Client002::Inited"_hash);
        this->hkSource2Client002_Init->Detach();
        return MulNX::Hook::Then::Return;
        }
    ).value();
    this->hkSource2Client002_Init->Attach();
    this->LogSucc(I18n("hook.attached", "Source2Client002::Init"));

    auto back = this->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Hud::ifShowSpeaker).Rdata();
    this->retAddrForShowSpeaker = reinterpret_cast<uintptr_t>(back) - 4;
    --this->needToLoadModules;
}
void CSController::OnEngine2Load(MulNX::Message& msg) {
    this->engine2 = CS2::Module::engine2(L"engine2.dll");
    this->Source2EngineToClient001 = this->engine2.GetProcAddressT<void* (const char*, int*)>("CreateInterface")("Source2EngineToClient001", nullptr);
    // demo
    this->GetDemo = IVClass::Assume(this->Source2EngineToClient001)->GetVFunc<void* ()>(68);
    // cmd
    this->hkSource2EngineToClient001_ExecuteCmd = MulNX::Hook::Create((uint8_t*)IVClass::Assume(this->Source2EngineToClient001)->GetVFuncPtr(50), [](MulNX::Hook* hk, RegContext* ctx) {
        static std::mutex mtx;
        std::lock_guard lock(mtx);
        hk->CallMaybeOrigin(0, ctx);
        return MulNX::Hook::Then::Return;
        }).value();
    this->hkSource2EngineToClient001_ExecuteCmd->Attach();
    this->LogSucc(I18n("hook.attached", "Source2EngineToClient001::ExecuteCmd"));

    // for show speaker
    this->hkSource2EngineToClient001_IsPlayingDemo = MulNX::Hook::Create((uint8_t*)IVClass::Assume(Source2EngineToClient001)->GetVFuncPtr(42), [this](MulNX::Hook* hk, RegContext* ctx) {
        auto returnAddress = *(uintptr_t*)hk->GetRawStackAddr(ctx);
        using Source2EngineToClient001_IsPlayingDemo_t = bool(*)(void*);
        *(bool*)(&ctx->rax) = reinterpret_cast<Source2EngineToClient001_IsPlayingDemo_t>(hk->pMaybeRawFunc)(reinterpret_cast<void*>(ctx->rcx));
        if (returnAddress == this->retAddrForShowSpeaker) {
            *(bool*)(&ctx->rax) = false;
        }
        // if (this->checkSource2EngineToClient001_IsPlayingDemo.Check(hk, ctx)) {
        //     if (this->Source2EngineToClient001ForceReturn) {
        //         *(bool*)(&ctx->rax) = this->Source2EngineToClient001ForceReturnValue;
        //     }
        // }
        return MulNX::Hook::Then::Return;
        }).value();
    this->hkSource2EngineToClient001_IsPlayingDemo->Attach();
    this->LogSucc(I18n("hook.attached", "Source2EngineToClient001::IsPlayingDemo"));

    --this->needToLoadModules;
}
void CSController::OnTier0Load(MulNX::Message& msg) {
    this->tier0 = MulNX::Memory::DllModule(L"tier0.dll");
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
        this->PublishAsync("Game/NewRound"_hash);
        OldRoundStartCount = currentStartCount;
    }
    // 玩家控制器，地图上从1到10
    int playerNum = 0;
    for (const auto& mod : this->ParticipateItCSModules) {
        mod->OnItBegin();
    }
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

        for (const auto& mod : this->ParticipateItCSModules) {
            mod->OnItPlayer(i, controller, pawn);
        }
        //MulNX::MWrite(pawn->m_entitySpottedState()->m_bSpotted(), true);

        if (playerNum <= 10) {
            std::unique_lock lock(this->smutex);
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
    for (const auto& mod : this->ParticipateItCSModules) {
        mod->OnItEnd();
    }
    return;
}

D_Player& CSController::GetPlayerMsg(int Index) {
    std::shared_lock lock(this->smutex);
    return this->CS2EBGameData.Players[Index];
}