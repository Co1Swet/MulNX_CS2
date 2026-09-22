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
            float topH = (ImGui::GetContentRegionAvail().y - ImGui::GetStyle().ItemSpacing.y) * 2.0f / 3.0f;
            auto c = MulNX::UI::RAIIChild("内容", ImVec2(0, topH), ImGuiChildFlags_Borders);
            bool InProject = false;
            if (this->PManager->ActiveProject) {
                InProject = true;
                ImGui::Text(I18n("camsys.proj.current", this->PManager->ActiveProject->Name).c_str());
            }
            else {
                ImGui::Text("还未打开任何运镜包，请打开");
            }

            ImGui::Separator();
            switch (selectedTab) {
            case 0:
                this->PManager->MenuProject();
                break;
            case 1:
                if (!InProject)break;
                this->SManager->MenuSolution();
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
    this->SManager = this->FindModule<SolutionManager>("SolutionManager");
    this->PManager = this->FindModule<ProjectManager>("ProjectManager");
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
        this->config = {};
        auto* PathManager = this->Path();
        PathManager->KeySetCurrent("kCurrentPack", {});
        PathManager->KeySetCurrent("kCamPacks", "CamPacks");
        if (this->ConfigLoad()) {
            this->ConfigApply();
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

bool CameraSystem::ConfigGenerate() {
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
    this->PManager->Config = this->config.ProjectCfg;
    this->SManager->Config = this->config.SolutionCfg;
    return true;
}

bool CameraSystem::HandleUpdateCSView(CS2::CViewSetup* viewSetup, const int& num, bool& camLeavePlayer) {
    this->Update();
    CameraSystemIO IO;
    bool needOverride = false;

    this->CamDrawer.Update(this->CS2View->GetViewMatrix(), this->CS2View->GetWinWidth(), this->CS2View->GetWinHeight());
    this->pCampathManager->HandleUpdate();
    if (this->SManager->HandleUpdate(&IO))needOverride = true;
    if (this->pCamPlayScheduler->HandleUpdate(&IO))needOverride = true;
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