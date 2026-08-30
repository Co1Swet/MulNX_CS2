#include "HookView.hpp"
#include "CSViewControlModuleBase.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <Intro/HookConsole/HookConsole.hpp>

void HookView::Window(MulNX::UICoordinator* uico) {
    auto w = MulNX::UI::RAIIWindow(I18n("镜头控制").c_str());
    if (!w)return;
    uico->CallbackCall("UI.CameraSetting"_hash, nullptr);
    if (!w.ShouldDraw())return;

    MulNX::UI::SliderFloat("roll调整", this->controlView.InputRoll, -179.99f, 179.99f);
    static auto* pGlobalFOV = this->CS2Con->GetCvar("fov_cs_debug")->GetPtr<float>();
    ImGui::SliderFloat("fov调整", pGlobalFOV, 0, 179.99f);
    if (ImGui::Button("一键归正")) {
        this->controlView.InputRoll.store(0, std::memory_order_release);
        *pGlobalFOV = 0;
    }
}

bool HookView::Init() {

    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        this->controlView.dofs.pNearBlurry = this->CS2Con->GetCvar("r_dof_override_near_blurry")->GetPtr<float>();
        this->controlView.dofs.pNearCrisp = this->CS2Con->GetCvar("r_dof_override_near_crisp")->GetPtr<float>();
        this->controlView.dofs.pFarCrisp = this->CS2Con->GetCvar("r_dof_override_far_crisp")->GetPtr<float>();
        this->controlView.dofs.pFarBlurry = this->CS2Con->GetCvar("r_dof_override_far_blurry")->GetPtr<float>();

        auto target = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Pos_CViewRenderer_VFuncsSubFunc_MaybeWriteView_CallIsPlayingDemo);
        this->hkPosCallIsPlayingDemo = MulNX::Hook::Create(target.Data(), [this](MulNX::Hook* Hook, RegContext* ctx) {
            this->HandleOverrideView((CS2::CViewSetup*)ctx->r14);
            return MulNX::Hook::Then::Continue;
            }, true).value();
        this->RegisterAttachHook(this->hkPosCallIsPlayingDemo, "Pos_CViewRenderer_VFuncsSubFunc_MaybeWriteView_CallIsPlayingDemo where r14 is *CViewSetup");

        this->SendUIRoot(this->GetName(), [this](auto uico, auto&&...) {return this->Window(uico);});
        });

    return true;
}

void HookView::HandleOverrideView(CS2::CViewSetup* viewSetup) {
    if (!this->pGlobalVars->SystemReady.load(std::memory_order_acquire)) {
        return;
    }

    // 同步窗口尺寸到ControlView
    this->controlView.WindowWidth.store(*viewSetup->pWidth(), std::memory_order_relaxed);
    this->controlView.WindowHeight.store(*viewSetup->pHeight(), std::memory_order_relaxed);

    // 执行roll覆盖，这是优先级最低的覆盖，保证运镜至少优先于此，且不影响于此
    viewSetup->pViewAngles()->z = this->controlView.InputRoll.load(std::memory_order_acquire);

    int num = 0;
    bool camLeavePlayer = false;
    for (auto& viewCtrlModule : this->viewControlModules) {
        if (viewCtrlModule->HandleUpdateCSView(viewSetup, num, camLeavePlayer))
            ++num;
    }

    if (this->cameraLeavePlayer.load(std::memory_order_acquire) != camLeavePlayer)
        this->cameraLeavePlayer.store(camLeavePlayer, std::memory_order_release);

    this->PublishSync("Hook/OnSetupView"_hash);

    // 记录视角数据
    {
        auto currentView = this->controlView.currentView.Write();

        currentView->position = *viewSetup->pViewOrigin();
        currentView->rotation = *viewSetup->pViewAngles();

        currentView->FOV = *viewSetup->pFov();
    }
}

float HookView::GetWinWidth()const {
    return this->controlView.WindowWidth.load(std::memory_order_relaxed);
}
float HookView::GetWinHeight()const {
    return this->controlView.WindowHeight.load(std::memory_order_relaxed);
}
float* HookView::GetViewMatrix() {
    return this->CS2->client.dwViewMatrix();
}
MulNX::Math::View HookView::GetView() {
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
Dofs HookView::GetDofs() {
    return this->controlView.dofs;
}

void HookView::spec_goto_ex(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& rot) {
    this->AsyncCommand(std::format("spec_goto {} {} {} {} {}", pos.x, pos.y, pos.z, rot.x, rot.y));
    this->controlView.InputRoll.store(rot.z, std::memory_order_release);
}
void HookView::SetDOF(const MulNX::Math::DOFParam& dof) {
    *this->controlView.dofs.pNearBlurry = dof.NearBlurry;
    *this->controlView.dofs.pNearCrisp = dof.NearCrisp;
    *this->controlView.dofs.pFarCrisp = dof.FarCrisp;
    *this->controlView.dofs.pFarBlurry = dof.FarBlurry;
}