#include "CameraSystem.hpp"
#include "CamSysExt.hpp"
#include <MulNX/MulNX.hpp>
#include <MulNX/Base/UI/UI.hpp>

CameraSystem* CamSysModule::CamSys() {
    static auto pCamSys = this->Core->ModuleManager()->FindModule<CameraSystem>("CameraSystem");
    return pCamSys;
}

bool CameraSystem::Menu(MulNX::UINode* node) {
    std::shared_lock lock(this->smutex);
    node->CallUINode("MenuWorkspace");

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
            node->CallUINode("MenuProject");
            break;
        case 1:// 解决方案菜单
            if (!InProject) {
                ImGui::Text(I18n("camsys.please_enter_proj").c_str());
                break;
            }
            node->CallUINode("MenuSolution");
            break;
        case 2:// 元素菜单
            if (!InProject) {
                ImGui::Text(I18n("camsys.please_enter_proj").c_str());
                break;
            }
            node->CallUINode("MenuElement");
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

    auto* PathManager = this->ISys().PathManager();
    if (PathManager->CreateKey("CurrentWorkspace", {},
        [this](MulNX::PathManager* PathManager)->bool {
            auto NewWorkspacePath = PathManager->PathGetFromKey("CurrentWorkspace");
            // 检验文件夹是否已存在
            if (!std::filesystem::exists(NewWorkspacePath)) {
                this->ISys().LogInfo("指定的工作区文件夹不存在，需创建新的工作区文件夹！  路径：" + NewWorkspacePath.string());
                // 创建文件夹
                try {
                    std::filesystem::create_directory(NewWorkspacePath);
                    // 子文件夹由项目创建时创建
                }
                catch (const std::filesystem::filesystem_error& e) {
                    this->ISys().LogError("创建工作区文件夹失败，错误信息：" + std::string(e.what()));
                    return false;
                }
                this->ISys().LogSucc("成功创建工作区文件夹，路径：" + NewWorkspacePath.string());
            }
            this->ISys().LogSucc("成功设置工作区路径为：" + NewWorkspacePath.string());
            return true;
        })) {
        auto Workspaces = this->ISys().PathGet("Workspaces");
        PathManager->KeyBindStatic("CurrentWorkspace", Workspaces);
    }
    this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {return this->Menu(node);});
    this->ISys()
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
        this->ISys().LogWarning("接收到播放停止消息");
        this->EManager->Preview_Disable();
        this->SManager->Playing_Disable();
        break;
    }
    case "Game/NewRound"_hash: {
        this->PManager->Playing_AutoCall(msg);
        break;
    }
    case "Command/SpecPlayer"_hash: {
        this->ISys().LogInfo("因为操作停止播放");
        this->EManager->Preview_Disable();
        this->SManager->Playing_Disable();
        break;
    }
    default:break;
    }
}

void CameraSystem::HandleUpdate() {
    this->EntryProcessMsg();
    if (this->pInputSystem->CheckWithPack(MulNX::KeyCheckPack{ true,false,false,true,'P',1 })) {
        this->ISys().PublishAsync("CameraSystem/Play/Shutdown"_hash);
    }
    this->CamDrawer.Update(this->AL3D->GetViewMatrix(), this->AL3D->GetWinWidth(), this->AL3D->GetWinHeight());
    this->EManager->HandleUpdate();
    this->SManager->HandleUpdate();
    this->PManager->HandleUpdate();
    this->WManager->HandleUpdate();
    return;
}