#include "CSController.hpp"

#include <MulNX/Base/UI/UI.hpp>
#include <MulNX/Base/Math/Translate/Translate.hpp>
#include <MulNXExtensions/CS2/PlayerHub/ProjectileTracker/ProjectileTracker.hpp>
#include <MulNXThirdParty/All_cs2_dumper.hpp>
#include <unordered_set>

bool CSController::Window(MulNX::UINode* node) {
    auto w = MulNX::UI::RAIIWindow("快捷操作", this->ShowWindow);
    if (!w)return true;

    auto tick = this->GetDemoTick();
    ImGui::Text("当前demotick：%d", tick);

    node->CallUINode("ViewController");

    if (ImGui::CollapsingHeader("时间控制")) {
        static float gameTimeScale = 1.0f;
        static float virtualTimeScale = 1.0f;
        ImGui::SliderFloat("游戏时间流速", &gameTimeScale, 0.0f, 5.0f);
        ImGui::SliderFloat("虚拟时间流速", &virtualTimeScale, 0.0f, 5.0f);

        if (ImGui::Button("启用时间虚拟化")) {
            this->ISys().AsyncCommand(std::format("host_timescale {}", gameTimeScale));
            this->Time()->RefreshVirtual(true, virtualTimeScale);
        }
        ImGui::SameLine();
        if (ImGui::Button("解除时间虚拟化")) {
            this->ISys().AsyncCommand(std::format("host_timescale 1"));
            this->Time()->RefreshVirtual(false, 1.0f);
        }
    }
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

    this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {return this->Window(node);});

    this->EnlistExecutors();

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

    return true;
}

void CSController::EnlistExecutors() {
    this->client = CS2::Module::Client(L"client.dll");
    this->engine2 = CS2::Module::engine2(L"engine2.dll");
    this->tier0 = MulNX::Memory::DllModule(L"tier0.dll");

    // 加载来自Source2EngineToClient001的模块
    this->Source2EngineToClient001 =
        this->engine2.GetProcAddressT<void* (const char*, int*)>("CreateInterface")
        ("Source2EngineToClient001", nullptr);
    this->executor = IVClass::Assume(this->Source2EngineToClient001)->GetVFunc<void(int, const char*, int)>(50);
    this->GetDemo = IVClass::Assume(this->Source2EngineToClient001)->GetVFunc<void* ()>(68);
    auto demo = this->GetDemo();
    this->GetDemoTick = IVClass::Assume(demo)->GetVFunc<int()>(3);
    this->IsPlayingDemo = IVClass::Assume(demo)->GetVFunc<bool()>(11);
    this->IsDemoPaused = IVClass::Assume(demo)->GetVFunc<bool()>(12);

    // 获取CvarSystem
    this->CvarSystem.Address =
        (uintptr_t)this->tier0.GetProcAddressT<void* (const char*, int*)>("CreateInterface")
        ("VEngineCvar007", nullptr);
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
float CSController::GetTime() {
    try {
        float time = MulNX::MRead(this->CSGlobalVars->fCurrentTime());
        // float timereal = MulNX::MRead(this->CSGlobalVars->fRealTime());
        // auto iTime2 = MulNX::MRead(this->CSGlobalVars->iTickCount());
        // auto fTime2 = static_cast<float>(iTime2) / 64.0f;
        // 经过验证，fCurrentTime更稳定一点
        return time;
    }
    catch (const std::runtime_error& e) {
        this->ISys().LogError("读取游戏时间失败");
        return 0;
    }

}
bool CSController::JumpTime(const float time) {
    int currentGameTick = this->Time()->GetReal() * 64;
    int currentDemoTick = this->GetDemoTick();

    int targetGameTick = static_cast<int>(time * 64);
    int deltaTick = currentGameTick - currentDemoTick;
    int tick = targetGameTick - deltaTick;

    std::string command = std::format("demo_gototick {}", tick);
    this->ISys().AsyncCommand(std::move(command));
    return true;
}
bool CSController::SpecPlayer(int IndexInMap) {
    this->ISys().AsyncCommand("spec_mode 2;spec_player " + std::to_string(this->CS2EBGameData.Players[IndexInMap].IndexInMap));
    return true;
}
D_Player& CSController::GetPlayerMsg(int Index) {
    //std::shared_lock lock(this->GetMtx());
    return this->CS2EBGameData.Players[Index];
}

MulNX::TimeBridge::TimeBridge(CSController* pCS2) : pCS2(pCS2) {
    this->startTime = std::chrono::steady_clock::now();
}

void MulNX::TimeBridge::update() {
    float time = this->pCS2->GetTime();
    if (time > this->lastRealTime) {
        this->lastRealTime = time;
    }
    else if (this->lastRealTime - time > 0.025f) {
        this->lastRealTime = time;
    }
    return;
}

bool MulNX::TimeBridge::RefreshVirtual(bool virtualTimePlaying, float scale) {
    this->update();
    this->refreshTime = this->lastRealTime;
    this->startTime = std::chrono::steady_clock::now();
    this->scale = scale;
    this->virtualTimePlaying = virtualTimePlaying;
    return true;
}

float MulNX::TimeBridge::GetReal() {
    this->update();
    return this->lastRealTime;
}

bool MulNX::TimeBridge::JumpReal(float time) {
    return this->pCS2->JumpTime(time);
}

bool MulNX::TimeBridge::JumpRealRel(float time) {
    return this->JumpReal(time + this->GetReal());
}

float MulNX::TimeBridge::GetVirtual() {
    // 这里不需要更新，因为虚拟时间的更新是由RefreshVirtual控制的，GetVirtual只负责计算当前的虚拟时间
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<float>(now - this->startTime).count();
    return this->refreshTime + elapsed * this->scale;
}

float MulNX::TimeBridge::Get() {
    return this->virtualTimePlaying ? this->GetVirtual() : this->GetReal();
}


MulNX::TimeBridge* CSController::Time() {
    return &this->timeBridge;
}