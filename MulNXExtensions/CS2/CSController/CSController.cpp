#include "CSController.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXThirdParty/All_cs2_dumper.hpp>

bool CSController::Init() {
    this->showWindow = true;
    this->ISys()
        .SubscribeAsync("Demo/GotoTick")
        .SubscribeAsync("Game/Command");

    this->ISys().SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {   
        this->client = CS2::Module::Client(L"client.dll");
        --this->needToLoadModules;
        });
    this->ISys().SubscribeSync("Hook/LoadLibraryExW/engine2.dll", [this](MulNX::Message& msg) {
        this->engine2 = CS2::Module::engine2(L"engine2.dll");
        // 加载来自Source2EngineToClient001的模块
        this->Source2EngineToClient001 =
            this->engine2.GetProcAddressT<void* (const char*, int*)>("CreateInterface")
            ("Source2EngineToClient001", nullptr);
        this->executor = IVClass::Assume(this->Source2EngineToClient001)->GetVFunc<void(int, const char*, int)>(50);
        this->GetDemo = IVClass::Assume(this->Source2EngineToClient001)->GetVFunc<void* ()>(68);
        --this->needToLoadModules;
        });
    this->ISys().SubscribeSync("Hook/LoadLibraryExW/tier0.dll", [this](MulNX::Message& msg) {
        this->tier0 = MulNX::Memory::DllModule(L"tier0.dll");
        // 获取CvarSystem
        this->CvarSystem.Address =
            (uintptr_t)this->tier0.GetProcAddressT<void* (const char*, int*)>("CreateInterface")
            ("VEngineCvar007", nullptr);
        --this->needToLoadModules;
        });
    this->ISys().SubscribeSync("Hook/LoadLibraryExW/panorama.dll", [this](MulNX::Message& msg) {
        this->panorama = MulNX::Memory::DllModule(L"panorama.dll");
        --this->needToLoadModules;
        });

    this->currentCoro = InitTask();
    this->currentCoro.resume();

    this->SendTask("CSControl", [this]()->bool {
        this->Update();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        return true;
        });

    return true;
}
MulNX::CoTask CSController::InitTask() {
    // 等待必要模块加载完成
    co_await this->WaitUntil([this]()->bool {return this->needToLoadModules.load() == 0;});

    this->SendTask("CSControl", [this]()->bool {
        try {
            this->Main();
        }
        catch (const std::runtime_error& e) {
            this->ISys().LogWarning("在更新数据时捕获到异常：" + std::string(e.what()));
        }
        return true;
        });

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