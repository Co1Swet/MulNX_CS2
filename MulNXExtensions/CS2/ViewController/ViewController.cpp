#include "ViewController.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/CS2/CSController/CSController.hpp>
#include <MulNXExtensions/CS2/CameraSystem/CameraSystem.hpp>
#include <MulNXExtensions/CS2/CameraSystem/CameraSystemIO/CameraSystemIO.hpp>
#include <MulNXExtensions/CS2/PlayerHub/ProjectileTracker/ProjectileTracker.hpp>

bool ViewController::Menu(MulNX::UINode* node) {

    MulNX::UI::SliderFloat("roll调整", this->controlView.InputRoll, -179.99f, 179.99f);
    static auto* pGlobalFOV = this->CS2()->GetCvarSystem().GetCvar("fov_cs_debug")->GetPtr<float>();
    ImGui::SliderFloat("fov调整", pGlobalFOV, 0, 179.99f);
    if (ImGui::Button("一键归正")) {
        this->controlView.InputRoll.store(0, std::memory_order_release);
        *pGlobalFOV = 0;
    }
    return true;
}

bool ViewController::Init() {
    this->pFreeCameraController = this->Core->ModuleManager()->FindModule<FreeCameraController>("FreeCameraController");
    this->pAdvancedViewController = this->Core->ModuleManager()->FindModule<AdvancedViewController>("AdvancedViewController");

    this->controlView.dofs.pNearBlurry = this->CS2()->GetCvarSystem().GetCvar("r_dof_override_near_blurry")->GetPtr<float>();
    this->controlView.dofs.pNearCrisp = this->CS2()->GetCvarSystem().GetCvar("r_dof_override_near_crisp")->GetPtr<float>();
    this->controlView.dofs.pFarCrisp = this->CS2()->GetCvarSystem().GetCvar("r_dof_override_far_crisp")->GetPtr<float>();
    this->controlView.dofs.pFarBlurry = this->CS2()->GetCvarSystem().GetCvar("r_dof_override_far_blurry")->GetPtr<float>();

    auto target = this->CS2()->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::CallIsPlayingDemo);
    this->hkPosCallIsPlayingDemo = MulNX::Hook::Create(target.Data(), 0, true,
        [this](RegContext* ctx, MulNX::Hook* Hook) {
            this->HandleOverrideView((CS2::CViewSetup*)ctx->rsi);
            return MulNX::Hook::Then::Continue;
        }).value();
    this->hkPosCallIsPlayingDemo->Attach();
    this->ISys().LogSucc(I18n("hook.attached", "Position On SomeWhere Call IsPlayingDemo, where rsi is pCViewSetup"));

    this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {return this->Menu(node);});


    return true;
}

void ViewController::HandleCameraSystemPlay(CS2::CViewSetup* viewSetup) {
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

void ViewController::HandleOverrideView(CS2::CViewSetup* viewSetup) {
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

void ViewController::HandleFreeCameraPath(const CameraSystemIO* const IO) {
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

float ViewController::GetWinWidth()const {
    return this->controlView.WindowWidth.load(std::memory_order_relaxed);
}
float ViewController::GetWinHeight()const {
    return this->controlView.WindowHeight.load(std::memory_order_relaxed);
}
float* ViewController::GetViewMatrix() {
    return this->CS2()->client.dwViewMatrix();
}
MulNX::Math::View ViewController::GetView() {
    MulNX::Math::View view;
    {
        auto read = this->controlView.currentView.Read();
        view.position = read->position;
        view.rotation = read->rotation;
        view.FOV = read->FOV;
    }

    view.dof.NearBlurry = *this->controlView.dofs.pNearBlurry;
    view.dof.NearCrisp = *this->controlView.dofs.pNearCrisp;
    view.dof.FarCrisp = *this->controlView.dofs.pFarCrisp;
    view.dof.FarBlurry = *this->controlView.dofs.pFarBlurry;

    return view;
}

void ViewController::spec_goto_ex(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& rot) {
    this->ISys().AsyncCommand(std::format("spec_goto {} {} {} {} {}", pos.x, pos.y, pos.z, rot.x, rot.y));
    this->controlView.InputRoll.store(rot.z, std::memory_order_release);
}
void ViewController::ClearViewOverride() {
    this->controlView.hasViewToGame.store(false, std::memory_order_release);
}
void ViewController::SetDOF(const MulNX::Math::DOFParam& dof) {
    *this->controlView.dofs.pNearBlurry = dof.NearBlurry;
    *this->controlView.dofs.pNearCrisp = dof.NearCrisp;
    *this->controlView.dofs.pFarCrisp = dof.FarCrisp;
    *this->controlView.dofs.pFarBlurry = dof.FarBlurry;
}
bool ViewController::CameraSystemIOOverride(const CameraSystemIO* const IO) {
    static float LastCallTime = IO->FrameGameTime;
    if (LastCallTime == IO->FrameGameTime) {
        return true;
    }
    LastCallTime = IO->FrameGameTime;

    this->HandleFreeCameraPath(IO);

    return true;
}