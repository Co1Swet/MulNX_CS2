#include "CSController.hpp"

#include <MulNX/Base/UI/UI.hpp>
#include <MulNX/Base/Math/Translate/Translate.hpp>
#include <MulNXExtensions/CS2/CameraSystem/CameraSystemIO/CameraSystemIO.hpp>
#include <MulNXExtensions/CS2/CameraSystem/CameraSystem.hpp>
#include <MulNXExtensions/CS2/PlayerHub/ProjectileTracker/ProjectileTracker.hpp>
#include <MulNXThirdParty/All_cs2_dumper.hpp>
#include <unordered_set>

bool CSController::Window(MulNX::UINode* node) {
    auto w = MulNX::UI::RAIIWindow("快捷操作", this->ShowWindow);
    if (!w)return true;
    MulNX::TransInfo info;
    info.pMatrix = this->GetViewMatrix();
    info.windowHeight = this->GetWinHeight();
    info.windowWidth = this->GetWinWidth();

    auto tick = this->GetDemoTick();
    ImGui::Text("当前demotick：%d", tick);

    MulNX::UI::SliderFloat("roll调整", this->controlView.InputRoll, -179.99f, 179.99f);
    auto* pGlobalFOV = this->CvarSystem.GetCvar("fov_cs_debug")->GetPtr<float>();
    ImGui::SliderFloat("fov调整", pGlobalFOV, 0, 179.99f);
    if (ImGui::Button("一键归正")) {
        this->controlView.InputRoll.store(0, std::memory_order_release);
        *pGlobalFOV = 0;
    }

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

void CSController::HandleCameraSystemPlay(CS2::CViewSetup* viewSetup) {
    // 加载来自摄像机系统的View
    if (this->controlView.hasViewToGame.load(std::memory_order_acquire)) {
        auto view = this->controlView.ViewToGame.Read();
        *viewSetup->pViewOrigin() = view->position;
        *viewSetup->pViewAngles() = view->rotation;

        if (view->FOV > 0.01f) {
            *viewSetup->pFov() = view->FOV;
        }
    }
}
void CSController::HandleOverrideView(CS2::CViewSetup* viewSetup) {
    if (this->GlobalVars->SystemReady.load(std::memory_order_acquire)) {
        this->Core->ModuleManager()->FindModule<CameraSystem>("CameraSystem")->HandleUpdate();
    }
    static auto* pProjectileTracker = this->Core->ModuleManager()->FindModule<ProjectileTracker>("ProjectileTracker");
    auto trckerView = pProjectileTracker->GetView();
    if (trckerView.has_value()) {
        *viewSetup->pViewOrigin() = trckerView.value().position;
        *viewSetup->pViewAngles() = trckerView.value().rotation;
    }

    // 同步窗口尺寸到ControlView
    this->controlView.WindowWidth.store(*viewSetup->pWidth(), std::memory_order_relaxed);
    this->controlView.WindowHeight.store(*viewSetup->pHeight(), std::memory_order_relaxed);

    // 执行roll覆盖，这是优先级最低的覆盖，保证运镜至少优先于此，且不影响于此
    viewSetup->pViewAngles()->z = this->controlView.InputRoll.load(std::memory_order_acquire);
    this->pAdvancedViewController->HandleUpdate(viewSetup);

    // 根据状态调用不同的视角控制逻辑
    // 自由摄像机优先级最高，其次是高级视角控制，最后是普通摄像机系统控制
    if (this->pFreeCameraController->HandleUpdate(viewSetup)) {
        this->pFreeCameraController->HandleOverrideView(viewSetup);
    }
    else if (this->pAdvancedViewController->HandleOverrideView(viewSetup)) {

    }
    else {
        this->HandleCameraSystemPlay(viewSetup);
    }

    // 记录视角数据
    {
        auto currentView = this->controlView.currentView.Write();

        currentView->position = *viewSetup->pViewOrigin();
        currentView->rotation = *viewSetup->pViewAngles();

        currentView->FOV = *viewSetup->pFov();
    }
}

bool CSController::Init() {
    this->ShowWindow = true;
    this->ISys()
        .SubscribeAsync("Demo/GotoTick")
        .SubscribeAsync("Game/Command");

    this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {return this->Window(node);});

    this->pAdvancedViewController = this->Core->ModuleManager()->FindModule<AdvancedViewController>("AdvancedViewController");
    this->pFreeCameraController = this->Core->ModuleManager()->FindModule<FreeCameraController>("FreeCameraController");

    this->EnlistExecutors();

    const auto& pattern = MulNX::CS2::Signatures::CallIsPlayingDemo;
    auto target = this->client.GetTextRegion().FindRegion(pattern);
    this->hkPosCallIsPlayingDemo = MulNX::Hook::Create(target.Data(), 0, true, [this](RegContext* ctx, MulNX::Hook* Hook) {
        this->HandleOverrideView((CS2::CViewSetup*)ctx->rsi);
        return MulNX::Hook::Then::Continue;
        }).value();
    this->hkPosCallIsPlayingDemo->Attach();
    this->ISys().LogSucc(I18n("hook.attached", "Position On SomeWhere Call IsPlayingDemo, where rsi is pCViewSetup"));

    this->controlView.dofs.pNearBlurry = this->CvarSystem.GetCvar("r_dof_override_near_blurry")->GetPtr<float>();
    this->controlView.dofs.pNearCrisp = this->CvarSystem.GetCvar("r_dof_override_near_crisp")->GetPtr<float>();
    this->controlView.dofs.pFarCrisp = this->CvarSystem.GetCvar("r_dof_override_far_crisp")->GetPtr<float>();
    this->controlView.dofs.pFarBlurry = this->CvarSystem.GetCvar("r_dof_override_far_blurry")->GetPtr<float>();

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

void CSController::HandleFreeCameraPath(const CameraSystemIO* const IO) {
    const auto& pos = IO->Frame.view.position;
    const auto& fov = IO->Frame.view.FOV;
    const auto& rot = IO->Frame.view.rotation;
    const auto& dof = IO->Frame.view.dof;

    {
        auto view = this->controlView.ViewToGame.Write();
        view->position = pos;
        view->FOV = fov;
        view->rotation = rot;
    }
    *this->controlView.dofs.pNearBlurry = dof.NearBlurry;
    *this->controlView.dofs.pNearCrisp = dof.NearCrisp;
    *this->controlView.dofs.pFarCrisp = dof.FarCrisp;
    *this->controlView.dofs.pFarBlurry = dof.FarBlurry;

    this->controlView.hasViewToGame.store(true, std::memory_order_release);
}
bool CSController::CameraSystemIOOverride(const CameraSystemIO* const IO) {
    static float LastCallTime = IO->FrameGameTime;
    if (LastCallTime == IO->FrameGameTime) {
        return true;
    }
    LastCallTime = IO->FrameGameTime;

    this->HandleFreeCameraPath(IO);

    return true;
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