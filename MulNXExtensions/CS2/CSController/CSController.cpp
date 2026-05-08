#include "CSController.hpp"

#include <MulNX/Base/UI/UI.hpp>
#include <MulNX/Base/Math/Translate/Translate.hpp>
#include <MulNXExtensions/CS2/PlayerHub/ProjectileTracker/ProjectileTracker.hpp>
#include <MulNXThirdParty/All_cs2_dumper.hpp>
#include <unordered_set>

bool CSController::Window(MulNX::UINode* node) {
    auto w = MulNX::UI::RAIIWindow("快捷操作", this->ShowWindow);
    if (!w)return true;

    node->CallUINode("ViewController");
    node->CallUINode("PlayerFlashController");
    node->CallUINode("AdvancedViewController");
    node->CallUINode("FreeCameraController");

    return true;
}

bool CSController::Init() {
    this->ShowWindow = true;
    this->ISys()
        .SubscribeAsync("Demo/GotoTick")
        .SubscribeAsync("Game/Command");

    this->client = CS2::Module::Client(L"client.dll");
    this->engine2 = CS2::Module::engine2(L"engine2.dll");
    this->tier0 = MulNX::Memory::DllModule(L"tier0.dll");

    // 加载来自Source2EngineToClient001的模块
    this->Source2EngineToClient001 =
        this->engine2.GetProcAddressT<void* (const char*, int*)>("CreateInterface")
        ("Source2EngineToClient001", nullptr);
    this->executor = IVClass::Assume(this->Source2EngineToClient001)->GetVFunc<void(int, const char*, int)>(50);
    this->GetDemo = IVClass::Assume(this->Source2EngineToClient001)->GetVFunc<void* ()>(68);

    // 获取CvarSystem
    this->CvarSystem.Address =
        (uintptr_t)this->tier0.GetProcAddressT<void* (const char*, int*)>("CreateInterface")
        ("VEngineCvar007", nullptr);

    this->SendTask("CSControl", [this]()->bool {
        try {
            this->Update();
            this->EntryProcessMsg();
            
        }
        catch (const std::runtime_error& e) {
            this->ISys().LogWarning("在更新数据时捕获到异常：" + std::string(e.what()));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        return true;
        });
    this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {return this->Window(node);});

    return true;
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

void CSController::Update() {
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
        auto* controller = this->client.GetBaseEntity(i)->As<CS2::CCSPlayerController>();
        if (!controller)continue;
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