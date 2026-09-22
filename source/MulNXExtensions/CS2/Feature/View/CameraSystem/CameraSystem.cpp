#include "CameraSystem.hpp"
#include "CamSysExt.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <Intro/HookView/HookView.hpp>

void CameraSystem::Window(MulNX::UICoordinator* uico) {
    auto w = MulNX::UI::RAIIWindow(I18n("ui.camera_system").c_str());
    if (!w || !w.ShouldDraw())return;
    std::shared_lock lock(this->smutex);

    // 进入工作区，显示工作区内容
    static int selectedTab = 0;
    // 左侧导航栏
    {
        auto c = MulNX::UI::RAIIChild("导航", ImVec2(150, 0), ImGuiChildFlags_Borders);
        if (ImGui::Selectable("运镜包管理", selectedTab == 0))
            selectedTab = 0;
        if (ImGui::Selectable("运镜宏管理", selectedTab == 1))
            selectedTab = 1;
        if (ImGui::Selectable("运镜轨道管理", selectedTab == 2))
            selectedTab = 2;
    }
    ImGui::SameLine();
    {
        auto right = MulNX::UI::RAIIChild("右侧");
        {
            std::shared_lock lock(this->pCamPackManager->smutex);
            float topH = (ImGui::GetContentRegionAvail().y - ImGui::GetStyle().ItemSpacing.y) * 2.0f / 3.0f;
            auto c = MulNX::UI::RAIIChild("内容", ImVec2(0, topH), ImGuiChildFlags_Borders);
            bool InProject = false;
            if (this->pCamPackManager->pActiveCamPack) {
                InProject = true;
                ImGui::Text(std::format("当前运镜包名：{}", this->pCamPackManager->pActiveCamPack->GetName()).c_str());
            }
            else {
                ImGui::Text("还未打开任何运镜包，请打开一个运镜包");
            }

            ImGui::Separator();
            switch (selectedTab) {
            case 0:
                this->pCamPackManager->Menu();
                break;
            case 1:
                if (!InProject)break;
                this->pCamMacroManager->Menu();
                break;
            case 2:
                if (!InProject)break;
                this->pCampathManager->Menu();
                break;
            }
        }
        ImGui::Separator();
        {
            auto c = MulNX::UI::RAIIChild("控制", ImVec2(0, 0), ImGuiChildFlags_Borders);
            this->pCamPlayScheduler->Menu();
        }
    }
}

bool CameraSystem::Init() {
    // 传递指针，注入依赖，提升性能，直接调用
    // 注意，本模块所有级别的管理器相互显示注入，其它服务借助Core隐式注入
    this->CamDrawer.Init(20.0, 30.0, 15.0, 10.0, IM_COL32(255, 0, 255, 255));
    this->pCampathManager = this->FindModule<CampathManager>("CampathManager");
    this->pCamMacroManager = this->FindModule<CamMacroManager>("CamMacroManager");
    this->pCamPackManager = this->FindModule<CamPackManager>("CamPackManager");
    this->pCamPlayScheduler = this->FindModule<CamPlayScheduler>("CamPlayScheduler");
    this->pIPCer = this->FindModule<MulNX::IPCer>("IPCer");

    this->Path()->CreateKey("kCamPacks", {}, [this](MulNX::PathManager* PathManager)->bool {
        return true;
        });
    auto dirCameraSystem = this->PathGet("CamPacks").parent_path();
    this->Path()->KeyBindStatic("kCamPacks", dirCameraSystem);
    this->SendUIRoot(this->GetName(), [this](auto uico, auto&&...) {return this->Window(uico);});

    (*this)
        .SubscribeAsync("Global/Save")
        .SubscribeAsync("Global/Save/Strong")
        .SubscribeAsync("Command/SpecPlayer")
        ;

    this->SubscribeSync("System/Init/End", [this](auto&&...) {
        auto* PathManager = this->Path();
        PathManager->KeySetCurrent("kCurrentPack", {});
        PathManager->KeySetCurrent("kCamPacks", "CamPacks");
        });

    return true;
}

void CameraSystem::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Global/Save"_hash: {
        this->PublishSync("CamSync/SaveAll"_hash);
        this->LogSucc("摄像机系统保存成功");
        break;
    }
    case "Command/SpecPlayer"_hash: {
        this->LogInfo("因为操作停止播放");
        this->PublishAsync("CamPlay/Clear"_hash);
        break;
    }
    default:break;
    }
}

bool CameraSystem::HandleUpdateCSView(CS2::CViewSetup* viewSetup, const int& num, bool& camLeavePlayer) {
    this->Update();
    CameraSystemIO IO;

    this->CamDrawer.Update(this->CS2View->GetViewMatrix(), this->CS2View->GetWinWidth(), this->CS2View->GetWinHeight());
    this->pCampathManager->HandleUpdate();
    this->pCamMacroManager->HandleUpdate();
    this->pCamPackManager->HandleUpdate();

    if (!this->pCamPlayScheduler->HandleUpdate(&IO))return false;
    camLeavePlayer = true;

    const auto& pos = IO.Frame.view.position;
    const auto& fov = IO.Frame.view.FOV;
    const auto& rot = IO.Frame.view.rotation;
    const auto& dof = IO.Frame.view.dof;

    *viewSetup->pViewOrigin() = pos;
    *viewSetup->pViewAngles() = rot;

    if (fov > 0.01f) {
        *viewSetup->pFov() = fov;
    }

    this->CS2View->SetDOF(dof);

    return true;
}