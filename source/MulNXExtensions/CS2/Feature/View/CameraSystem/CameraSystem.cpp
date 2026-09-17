#include "CameraSystem.hpp"
#include "CamSysExt.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <Intro/HookView/HookView.hpp>

void CameraSystem::Window(MulNX::UICoordinator* uico) {
    auto w = MulNX::UI::RAIIWindow(I18n("ui.camera_system").c_str());
    if (!w || !w.ShouldDraw())return;
    std::shared_lock lock(this->smutex);

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
            this->PManager->MenuProject();
            break;
        case 1:// 解决方案菜单
            if (!InProject) {
                ImGui::Text(I18n("camsys.please_enter_proj").c_str());
                break;
            }
            this->SManager->MenuSolution();
            break;
        case 2:// 元素菜单
            if (!InProject) {
                ImGui::Text(I18n("camsys.please_enter_proj").c_str());
                break;
            }
            this->EManager->MenuElement();
            break;
        }
    }
}

bool CameraSystem::Init() {
    // 传递指针，注入依赖，提升性能，直接调用
    // 注意，本模块所有级别的管理器相互显示注入，其它服务借助Core隐式注入
    this->CamDrawer.Init(20.0, 30.0, 15.0, 10.0, IM_COL32(255, 0, 255, 255));
    this->EManager = this->FindModule<ElementManager>("ElementManager");
    this->SManager = this->FindModule<SolutionManager>("SolutionManager");
    this->PManager = this->FindModule<ProjectManager>("ProjectManager");
    this->pIPCer = this->FindModule<MulNX::IPCer>("IPCer");

    auto* PathManager = this->Path();
    PathManager->CreateKey("CurrentWorkspace", {}, [this](MulNX::PathManager* PathManager)->bool {
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
        });
    auto Workspaces = this->PathGet("Workspaces").parent_path();
    PathManager->KeyBindStatic("CurrentWorkspace", Workspaces);
    this->SendUIRoot(this->GetName(), [this](auto uico, auto&&...) {return this->Window(uico);});
    (*this)
        .SubscribeAsync("Global/Save")
        .SubscribeAsync("Global/Save/Strong")
        .SubscribeAsync("Command/SpecPlayer")
        .SubscribeAsync("Game/NewRound")
        .SubscribeAsync("CameraSystem/Play/Shutdown");

    this->SubscribeSync("System/Init/End", [this](auto&&...) {
        // 清空旧存储
        this->PManager->Project_ClearAll();
        // 制作指针
        this->config = {};
        auto* PathManager = this->Path();

        PathManager->KeySetCurrent("CurrentProject", {});
        PathManager->KeySetCurrent("CurrentWorkspace", "Packs");
        if (this->ConfigLoad()) {
            this->ConfigApply();
        }
        // 自动加载所有项目到内存中
        auto ProPath = PathManager->PathGetFromKey("CurrentWorkspace");
        std::vector<std::string> ProjectsNames = this->pIPCer->GetProjectsNames(ProPath);
        if (!ProjectsNames.empty()) {
            for (const auto& ProjectName : ProjectsNames) {
                std::filesystem::path ProjectPath = PathManager->PathGetFromKey("CurrentWorkspace") / ProjectName;
                this->PManager->Project_Load(ProjectPath, ProjectName);
            }
        }
        });

    return true;
}

void CameraSystem::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Global/Save"_hash: {
        //生成配置文件
        if (!this->ConfigGenerate()) {
            return;
        }
        //保存配置文件
        if (!this->ConfigSave()) {
            return;
        }
        //保存当前活跃项目
        if (!this->PManager->Project_Save()) {
            return;
        }
        this->LogSucc("摄像机系统配置保存成功");
        break;
    }
    case "CameraSystem/Play/Shutdown"_hash: {
        this->LogWarning("接收到播放停止消息");
        this->PublishSync("CamSync/Play/Shutdown"_hash);
        break;
    }
    case "Game/NewRound"_hash: {
        this->PManager->Playing_AutoCall(msg);
        break;
    }
    case "Command/SpecPlayer"_hash: {
        this->LogInfo("因为操作停止播放");
        this->PublishSync("CamSync/Play/Shutdown"_hash);
        break;
    }
    default:break;
    }
}

bool CameraSystem::ConfigGenerate() {
    this->config.ElementCfg = this->EManager->Config;
    this->config.ProjectCfg = this->PManager->Config;
    this->config.SolutionCfg = this->SManager->Config;
    return true;
}
bool CameraSystem::ConfigSave() {
    auto [ok, msg] = this->config.Save(this->PathGet("Config"));
    if (!ok) {
        this->LogError(std::move(msg));
        return false;
    }
    this->LogSucc(std::move(msg));
    return true;
}

bool CameraSystem::ConfigLoad() {
    auto path = this->PathGet("Config");
    // 拼接完整路径
    std::filesystem::path FullPath = path / ("Config.yaml");
    // 检查文件本身存在性
    if (!std::filesystem::exists(FullPath)) {
        this->LogWarning(I18n("result.error_no_file", FullPath.string()));
        return false;
    }
    // 输出调试信息
    this->LogInfo(I18n("action.try_load_config", FullPath.string()));

    try {
        YAML::Node root = YAML::LoadFile(FullPath.string());

        auto config = root["config"];

        auto elements = config["elements"];
        this->config.ElementCfg.PreviewDraw = elements["PreviewDraw"].as<bool>();
        this->config.ElementCfg.PreviewOverride = elements["PreviewOverride"].as<bool>();

        auto solutions = config["solutions"];
        this->config.SolutionCfg.SolutionShortcutEnable = solutions["shortcutEnable"].as<bool>();
        this->config.SolutionCfg.PlayingDraw = solutions["PlayingDraw"].as<bool>();
        this->config.SolutionCfg.PlayingOverride = solutions["PlayingOverride"].as<bool>();

        auto projects = config["projects"];
        this->config.ProjectCfg.ProjectShortcutEnable = projects["ProjectShortcutEnable"].as<bool>();

        this->LogSucc(I18n("result.cfg_load_succ", FullPath.string()));
        return true;
    }
    catch (const YAML::Exception& e) {
        this->LogError(I18n("result.cfg_load_error", e.what()));
        return false;
    }
}
bool CameraSystem::ConfigApply() {
    this->EManager->Config = this->config.ElementCfg;
    this->PManager->Config = this->config.ProjectCfg;
    this->SManager->Config = this->config.SolutionCfg;
    return true;
}

bool CameraSystem::HandleUpdateCSView(CS2::CViewSetup* viewSetup, const int& num, bool& camLeavePlayer) {
    this->Update();
    CameraSystemIO IO;
    bool needOverride = false;

    this->CamDrawer.Update(this->CS2View->GetViewMatrix(), this->CS2View->GetWinWidth(), this->CS2View->GetWinHeight());
    if (this->EManager->HandleUpdate(&IO))needOverride = true;
    if (this->SManager->HandleUpdate(&IO))needOverride = true;
    this->PManager->HandleUpdate();

    if (!needOverride)return false;
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

std::pair<bool, std::string> Config::Save(const std::filesystem::path& FolderPath) {
    // 检查文件路径和名称存在性
    if (FolderPath.empty())return { false,"文件夹路径为空，无法保存工作区配置文件！" };

    // 拼接完整路径
    std::filesystem::path FullPath = FolderPath / ("Config.yaml");
    try {
        YAML::Node root;

        auto config = root["config"];
        auto elements = config["elements"];
        elements["PreviewDraw"] = this->ElementCfg.PreviewDraw;
        elements["PreviewOverride"] = this->ElementCfg.PreviewOverride;

        auto solutions = config["solutions"];
        solutions["shortcutEnable"] = this->SolutionCfg.SolutionShortcutEnable;
        solutions["PlayingDraw"] = this->SolutionCfg.PlayingDraw;
        solutions["PlayingOverride"] = this->SolutionCfg.PlayingOverride;

        auto projects = config["projects"];
        projects["ProjectShortcutEnable"] = this->ProjectCfg.ProjectShortcutEnable;

        std::ofstream fout(FullPath);
        fout << root;
        fout.close();

        return { true,"成功保存工作区配置文件到文件！ 文件路径：" + FullPath.string() };
    }
    catch (const YAML::Exception& e) {
        return { false,"在尝试保存工作区时发生异常：" + std::string(e.what()) };
    }
}