#include "WorkspaceManager.hpp"

#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/CS2/CameraSystem/ElementManager/ElementManager.hpp>
#include <MulNXExtensions/CS2/CameraSystem/SolutionManager/SolutionManager.hpp>
#include <MulNXExtensions/CS2/CameraSystem/ProjectManager/ProjectManager.hpp>

#include <MulNXThirdParty/All_pugixml.hpp>

bool WorkspaceManager::MenuWorkspace(MulNX::UINode* node) {
    std::shared_lock lock(this->CamSys->smutex);
    // 顶部：工作区信息（始终显示）
    auto c = MulNX::UI::RAIIChild("工作区面板", ImVec2(0, 150), true);
    // 工作区状态
    ImGui::Text(I18n("camsys.ws.enter_status",
        this->InWorkspace ? I18n("text.entered") : I18n("text.unentered")).c_str());
    // 未进入工作区允许打开默认工作区
    if (!this->InWorkspace) {
        ImGui::SameLine();
        if (ImGui::Button(I18n("camsys.ws.open_default").c_str())) {
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CamereSystem/Workspace/Set"_hash);
            rp->str1 = "DefaultWorkspace";
            this->ISys().PublishAsync(std::move(msg));
        }
    }
    // 打开工作区后允许保存工作区
    if (this->InWorkspace) {
        ImGui::Text(I18n("camsys.ws.current_workspace", this->CurrentWorkspace->Name).c_str());
        ImGui::SameLine();
        if (ImGui::Button(I18n("camsys.ws.save").c_str())) {
            this->ISys().PublishAsync("CameraSystem/Workspace/Save"_hash);
        }
    }
    // 详情信息
    ImGui::Separator();
    auto c2 = MulNX::UI::RAIIChild("工作区详情菜单");
    if (ImGui::CollapsingHeader(I18n("text.detail_info").c_str())) {
        static std::string TargetWorkspaceName{};
        ImGui::Text(I18n("camsys.ws.target_name").c_str());
        ImGui::SameLine();
        ImGui::InputText("##TargetWorkspaceName", &TargetWorkspaceName);
        ImGui::SameLine();
        if (ImGui::Button(I18n("text.change").c_str())) {
            if (TargetWorkspaceName.empty()) {
                this->ISys().LogError(I18n("result.error_empty_name"));
                return true;
            }
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CamereSystem/Workspace/Set"_hash);
            rp->str1 = std::move(TargetWorkspaceName);
            this->ISys().PublishAsync(std::move(msg));
        }
        if (!this->CurrentWorkspace) {// 无工作区
            ImGui::Text(I18n("camsys.ws.current_null").c_str());
            return true;
        }
    }
    return true;
}

bool WorkspaceManager::Init() {
    this->EManager = this->Core->ModuleManager()->FindModule<ElementManager>("ElementManager");
    this->SManager = this->Core->ModuleManager()->FindModule<SolutionManager>("SolutionManager");
    this->PManager = this->Core->ModuleManager()->FindModule<ProjectManager>("ProjectManager");
    this->pIPCer = this->Core->ModuleManager()->FindModule<MulNX::IPCer>("IPCer");

    this->ISys().SendUINode("MenuWorkspace", [this](MulNX::UINode* node) {return this->MenuWorkspace(node);});

    this->ISys()
        .SubscribeAsync("CamereSystem/Workspace/Set")
        .SubscribeAsync("CameraSystem/Workspace/Save");

    return true;
}

void WorkspaceManager::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "CamereSystem/Workspace/Set"_hash: {
        auto name = msg.asp.get<MulNX::NetExt>()->str1;
        std::unique_lock lock(this->CamSys->smutex);
        this->Workspace_Set(name);
        break;
    }
    case "CameraSystem/Workspace/Save"_hash: {
        std::unique_lock lock(this->CamSys->smutex);
        this->Workspace_Save();
        break;
    }
    }
}

void WorkspaceManager::HandleUpdate() {
    this->Update();
}

bool WorkspaceManager::Workspace_Save() {
    //生成配置文件
    if (!this->Workspace_ConfigGenerate()) {
        return false;
    }
    //保存配置文件
    if (!this->Workspace_ConfigSave()) {
        return false;
    }
    //保存当前活跃项目
    if (!this->PManager->Project_Save()) {
        return false;
    }
    this->ISys().LogSucc(I18n("result.save_success"));
    return true;
}
bool WorkspaceManager::Workspace_Set(const std::string& Name) {
    this->InWorkspace = false;
    // 清空旧存储
    this->EManager->Element_ClearAll();
    this->SManager->Solution_ClearAll();
    this->PManager->Project_ClearAll();
    // 制作指针
    this->CurrentWorkspace = std::make_unique<Workspace>(Name);
    auto* PathManager = this->ISys().Path();

    PathManager->KeySetCurrent("CurrentProject", {});
    PathManager->KeySetCurrent("CurrentWorkspace", Name);
    // 获取配置信息
    if (this->Workspace_ConfigLoad(PathManager->PathGetFromKey("CurrentWorkspace"))) {
        this->Workspace_ConfigApply();
    }
    // 进入工作区
    this->InWorkspace = true;
    // 自动加载所有项目到内存中
    auto ProPath = PathManager->PathGetFromKey("CurrentWorkspace");
    std::vector<std::string> ProjectsNames = this->pIPCer->GetProjectsNames(ProPath);
    if (!ProjectsNames.empty()) {
        for (const auto& ProjectName : ProjectsNames) {
            std::filesystem::path ProjectPath = PathManager->PathGetFromKey("CurrentWorkspace") / ProjectName;
            this->PManager->Project_Load(ProjectPath, ProjectName);
        }
    }
    return true;
}

//config相关

bool WorkspaceManager::Workspace_ConfigGenerate() {
    if (!this->CurrentWorkspace) {
        return false;
    }
    WorkspaceConfig& Config = this->CurrentWorkspace->Config;
    Config.ElementCfg = this->EManager->Config;
    Config.ProjectCfg = this->PManager->Config;
    Config.SolutionCfg = this->SManager->Config;
    return true;
}
bool WorkspaceManager::Workspace_ConfigSave() {
    if (!this->CurrentWorkspace) {
        return false;
    }
    auto [ok, msg] = this->CurrentWorkspace->Save(this->ISys().Path()->PathGetFromKey("CurrentWorkspace"));
    if (!ok) {
        this->ISys().LogError(std::move(msg));
        return false;
    }
    this->ISys().LogSucc(std::move(msg));
    return true;
}
bool WorkspaceManager::Workspace_ConfigLoad(const std::filesystem::path& WorkspacePath) {
    if (!this->CurrentWorkspace) {
        return false;
    }
    // 检查文件路径和名称存在性
    if (WorkspacePath.empty()) {
        this->ISys().LogError(I18n("result.error_no_path", WorkspacePath.string()));
        return false;
    }
    // 拼接完整路径
    std::filesystem::path FullPath = WorkspacePath / ("WorkspaceConfig.yaml");
    // 检查文件本身存在性
    if (!std::filesystem::exists(FullPath)) {
        this->ISys().LogWarning(I18n("result.error_no_file", FullPath.string()));
        return false;
    }
    // 输出调试信息
    this->ISys().LogInfo(I18n("action.try_load_config", FullPath.string()));

    try {
        YAML::Node root = YAML::LoadFile(FullPath.string());
        WorkspaceConfig& Config = this->CurrentWorkspace->Config;

        auto config = root["config"];

        auto elements = config["elements"];
        Config.ElementCfg.PreviewDraw = elements["PreviewDraw"].as<bool>();
        Config.ElementCfg.PreviewOverride = elements["PreviewOverride"].as<bool>();

        auto solutions = config["solutions"];
        Config.SolutionCfg.SolutionShortcutEnable = solutions["shortcutEnable"].as<bool>();
        Config.SolutionCfg.PlayingDraw = solutions["PlayingDraw"].as<bool>();
        Config.SolutionCfg.PlayingOverride = solutions["PlayingOverride"].as<bool>();

        auto projects = config["projects"];
        Config.ProjectCfg.ProjectShortcutEnable = projects["ProjectShortcutEnable"].as<bool>();

        this->ISys().LogSucc(I18n("result.cfg_load_succ", FullPath.string()));
        return true;
    }
    catch (const YAML::Exception& e) {
        this->ISys().LogError(I18n("result.cfg_load_error", e.what()));
        return false;
    }
}
bool WorkspaceManager::Workspace_ConfigApply() {
    if (!this->CurrentWorkspace) {
        return false;
    }
    WorkspaceConfig& Config = this->CurrentWorkspace->Config;
    this->EManager->Config = Config.ElementCfg;
    this->PManager->Config = Config.ProjectCfg;
    this->SManager->Config = Config.SolutionCfg;
    return true;
}