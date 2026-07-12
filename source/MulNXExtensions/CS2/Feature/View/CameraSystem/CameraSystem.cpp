#include "CameraSystem.hpp"
#include "CamSysExt.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <Intro/HookView/HookView.hpp>

bool CameraSystem::Menu(MulNX::UICoordinator* uico) {
    std::shared_lock lock(this->smutex);
    uico->CallUINode("MenuWorkspace");

    if (!this->WManager->InWorkspace) {
        // 如果不在工作区，显示提示信息
        auto c = MulNX::UI::RAIIChild("提示", ImVec2(0, 0), true);
        ImGui::Text(I18n("camsys.please_enter_ws").c_str());
        return true;
    }

    // 进入工作区，显示工作区内容
    static int SelectedTab = 0;
    // 左侧导航栏
    {
        auto c = MulNX::UI::RAIIChild("导航", ImVec2(150, 0), true);
        if (ImGui::Selectable(I18n("camsys.tab_proj").c_str(), SelectedTab == 0))
            SelectedTab = 0;
        if (ImGui::Selectable(I18n("camsys.tab_sol").c_str(), SelectedTab == 1))
            SelectedTab = 1;
        if (ImGui::Selectable(I18n("camsys.tab_elem").c_str(), SelectedTab == 2))
            SelectedTab = 2;
    }
    ImGui::SameLine();
    {
        // 右侧三类控制区
        auto c = MulNX::UI::RAIIChild("内容", ImVec2(0, 0), true);
        bool InProject = false;
        if (this->PManager->ActiveProject) {
            InProject = true;
            ImGui::Text(I18n("camsys.proj.current", this->PManager->ActiveProject->Name).c_str());
        }
        else {
            ImGui::Text(I18n("camsys.please_enter_proj").c_str());
        }

        ImGui::Separator();
        switch (SelectedTab) {
        case 0:// 项目菜单
            uico->CallUINode("MenuProject");
            break;
        case 1:// 解决方案菜单
            if (!InProject) {
                ImGui::Text(I18n("camsys.please_enter_proj").c_str());
                break;
            }
            uico->CallUINode("MenuSolution");
            break;
        case 2:// 元素菜单
            if (!InProject) {
                ImGui::Text(I18n("camsys.please_enter_proj").c_str());
                break;
            }
            uico->CallUINode("MenuElement");
            break;
        }
    }
    
    return true;
}

bool CameraSystem::Init() {
    // 传递指针，注入依赖，提升性能，直接调用
    // 注意，本模块所有级别的管理器相互显示注入，其它服务借助Core隐式注入
    this->CamDrawer.Init(20.0, 30.0, 15.0, 10.0, IM_COL32(255, 0, 255, 255));
    this->EManager = this->Core->ModuleManager()->FindModule<ElementManager>("ElementManager");
    this->SManager = this->Core->ModuleManager()->FindModule<SolutionManager>("SolutionManager");
    this->PManager = this->Core->ModuleManager()->FindModule<ProjectManager>("ProjectManager");
    this->WManager = this->Core->ModuleManager()->FindModule<WorkspaceManager>("WorkspaceManager");

    auto* PathManager = this->Path();
    if (PathManager->CreateKey("CurrentWorkspace", {},
        [this](MulNX::PathManager* PathManager)->bool {
            auto NewWorkspacePath = PathManager->PathGetFromKey("CurrentWorkspace");
            // 检验文件夹是否已存在
            if (!std::filesystem::exists(NewWorkspacePath)) {
                this->LogInfo("指定的工作区文件夹不存在，需创建新的工作区文件夹！  路径：" + NewWorkspacePath.string());
                // 创建文件夹
                try {
                    std::filesystem::create_directory(NewWorkspacePath);
                    // 子文件夹由项目创建时创建
                }
                catch (const std::filesystem::filesystem_error& e) {
                    this->LogError("创建工作区文件夹失败，错误信息：" + std::string(e.what()));
                    return false;
                }
                this->LogSucc("成功创建工作区文件夹，路径：" + NewWorkspacePath.string());
            }
            this->LogSucc("成功设置工作区路径为：" + NewWorkspacePath.string());
            return true;
        })) {
        auto Workspaces = this->PathGet("Workspaces");
        PathManager->KeyBindStatic("CurrentWorkspace", Workspaces);
    }
    this->SendUINode(this->GetName(), [this](auto uico,auto&&...) {return this->Menu(uico);});
    (*this)
        .SubscribeAsync("Global/Save")
        .SubscribeAsync("Global/Save/Strong")
        .SubscribeAsync("Command/SpecPlayer")
        .SubscribeAsync("Game/NewRound")
        .SubscribeAsync("CameraSystem/Play/Shutdown");
    return true;
}

void CameraSystem::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Global/Save"_hash: {
        this->WManager->Workspace_Save();
        break;
    }
    case "CameraSystem/Play/Shutdown"_hash: {
        this->LogWarning("接收到播放停止消息");
        this->EManager->Preview_Disable();
        this->SManager->Playing_Disable();
        break;
    }
    case "Game/NewRound"_hash: {
        this->PManager->Playing_AutoCall(msg);
        break;
    }
    case "Command/SpecPlayer"_hash: {
        this->LogInfo("因为操作停止播放");
        this->EManager->Preview_Disable();
        this->SManager->Playing_Disable();
        break;
    }
    default:break;
    }
}

bool CameraSystem::HandleUpdate(CS2::CViewSetup* viewSetup, const int& num) {
    this->Update();
    CameraSystemIO IO;
    bool needOverride = false;

    this->CamDrawer.Update(this->CS2View->GetViewMatrix(), this->CS2View->GetWinWidth(), this->CS2View->GetWinHeight());
    if (this->EManager->HandleUpdate(&IO))needOverride = true;
    if (this->SManager->HandleUpdate(&IO))needOverride = true;
    this->PManager->HandleUpdate();
    this->WManager->HandleUpdate();

    if (!needOverride)return false;

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