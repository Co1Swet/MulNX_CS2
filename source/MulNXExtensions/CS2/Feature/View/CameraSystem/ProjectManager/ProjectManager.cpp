#include"ProjectManager.hpp"
#include <CameraSystem/SolutionManager/SolutionManager.hpp>

bool ProjectManager::Init() {
    this->pIPCer = this->Core->ModuleManager()->FindModule<MulNX::IPCer>("IPCer");

    this->SendUIRoot(this->GetName(), [this](auto&&...) {return this->UINodeFunc();});

    auto* PathManager = this->Path();
    PathManager->CreateKey("CurrentPack", {}, [this](MulNX::PathManager* PathManager)->bool {
        auto NewProjectPath = PathManager->PathGetFromKey("CurrentPack");
        // 检验文件夹是否已存在
        if (!std::filesystem::exists(NewProjectPath)) {
            this->LogInfo("指定的项目文件夹不存在，需创建新的项目文件夹！  路径：" + NewProjectPath.string());
            //创建文件夹
            try {
                std::filesystem::create_directory(NewProjectPath);
                //创建子文件夹
                std::filesystem::create_directory(NewProjectPath / "Elements");
                std::filesystem::create_directory(NewProjectPath / "Solutions");
            }
            catch (const std::filesystem::filesystem_error& e) {
                this->LogError("创建项目文件夹失败，错误信息：" + std::string(e.what()));
                return false;
            }
            this->LogSucc("成功创建项目文件夹，路径：" + NewProjectPath.string());
            return true;
        }
        this->LogSucc("成功设置项目路径为：" + NewProjectPath.string());
        return true;
        });
    PathManager->KeyBindDynamic("CurrentPack", "Packs");

    (*this)
        .SubscribeAsync("Game/NewRound");

    this->SubscribeSync("System/Init/End", [this](auto&&...) {
        // 自动加载所有项目到内存中
        auto ProPath = this->Path()->PathGetFromKey("Packs");
        std::vector<std::string> ProjectsNames = this->pIPCer->GetDirNamesByPath(ProPath);
        if (!ProjectsNames.empty()) {
            for (const auto& ProjectName : ProjectsNames) {
                std::filesystem::path ProjectPath = ProPath / ProjectName;
                this->Project_Load(ProjectPath, ProjectName);
            }
        }
        });

    this->SubscribeSync("CamSync/SaveAll", [this](auto&&...) {
        this->Project_Save();
        });

    return true;
}
void ProjectManager::HandleUpdate() {
    this->Update();
    if (!this->Config.ProjectShortcutEnable)return;
    for (const auto& [name, project] : this->projects) {
        if (this->pInputSystem->CheckWithPack(project->KCPack)) {
            this->Project_Apply(project);
        }
    }
}

void ProjectManager::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Game/NewRound"_hash: {
        if (!this->ActiveProject)break;
        const std::vector<std::string>& OnNewRound = this->ActiveProject->OnNewRound;
        if (OnNewRound.empty()) {
            this->LogWarning("无新回合解决方案可尝试调用");
            break;
        }
        int temp = rand() % OnNewRound.size();
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CameraSystem/Solution/Play"_hash);
        rp->str1 = OnNewRound[temp];
        this->PublishAsync(std::move(msg));
        break;
    }
    case "Game/RoundEnd"_hash: {
        // const std::vector<std::string>& OnEnd = this->ActiveProject->OnRoundEnd;
        // if (OnEnd.empty()) {
        //     this->LogWarning("无回合结束解决方案可尝试调用");
        //     return false;
        // }
        // int temp = rand() % OnEnd.size();
        // return this->SManager->Playing_SetSolution(OnEnd[temp]);
    }
    }
}

bool ProjectManager::Project_Delete(const std::string& name) {
    auto it = this->projects.find(name);
    if (it == this->projects.end()) {
        return true;
    }
    this->projects.erase(it);
    return true;
}
bool ProjectManager::Project_ClearAll() {
    this->projects.clear();
    this->ActiveProject = nullptr;
    return true;
}

bool ProjectManager::Project_Create(const std::string& name) {
    //检查是否已存在同名项目
    if (this->projects.find(name) != this->projects.end()) {
        this->LogError("项目名已占用！ 项目名：" + name);
        return false;
    }
    //创建项目指针
    std::shared_ptr<Project> CreateProject = std::make_shared<Project>(name);
    CreateProject->Refresh();
    //添加进项目组
    this->projects[name] = std::move(CreateProject);
    this->LogSucc("成功创建项目：" + name);
    return true;
}
bool ProjectManager::Project_Refresh() {
    //先验证有没有活跃项目
    if (!this->ActiveProject) {
        return false;
    }
    this->ActiveProject->Refresh();
    //刷新完毕
    return true;
}
bool ProjectManager::Project_Save() {
    if (!this->Project_Refresh()) {
        return false;
    }    
    // 保存项目到磁盘
    std::filesystem::path Path = this->Path()->PathGetFromKey("Packs") / this->ActiveProject->Name;
    auto [ok, msg] = this->ActiveProject->Save(Path);
    if (ok) {
        this->LogSucc(std::move(msg));
        return true;
    }
    else {
        this->LogError(std::move(msg));
        return false;
    }
}

bool ProjectManager::Project_Apply(const std::shared_ptr<Project> Project) {
    //先尝试保存当前活跃项目
    this->Project_Save();
    //切换项目
    this->ActiveProject = Project;
    //清空旧元素，防止冲突
    this->PublishSync("CamSync/Clear"_hash);
    if (!this->Path()->KeySetCurrent("CurrentPack", Project->Name)) {
        this->LogError("尝试切换到项目时出现问题，设置项目文件夹路径失败！");
        return false;
    }
    this->PublishSync("CamSync/Load"_hash);
    this->ActiveProject = Project;
    this->LogSucc("已切换至项目" + Project->Name);
    return true;
}
bool ProjectManager::Project_Load(const std::filesystem::path& ProjectPath, const std::string& yamlName) {
    // 检查文件路径和名称存在性
    if (ProjectPath.empty() || yamlName.empty()) {
        this->LogError("文件夹路径或文件名为空，无法从文件加载项目！");
        return false;
    }

    // 拼接完整路径
    std::filesystem::path FullPath = ProjectPath / (yamlName + ".yaml");

    // 输出调试信息
    this->LogInfo("尝试从文件加载项目，文件路径：" + FullPath.string());

    // 检查文件本身存在性
    if (!std::filesystem::exists(FullPath)) {
        this->LogError("文件不存在！文件路径：" + FullPath.string());
        return false;
    }

    try {
        YAML::Node root = YAML::LoadFile(FullPath.string());
        std::string loadProjectName = root["name"].as<std::string>();
        //检查是否存在同名项目
        if (this->projects.find(loadProjectName) != this->projects.end()) {
            this->LogError("项目名已占用，无法从文件加载项目！ 项目名：" + std::move(loadProjectName));
            return false;
        }
        auto loadProject = std::make_shared<Project>(loadProjectName);
        loadProject->KCPack = root["KCP"].as<MulNX::KeyCheckPack>();
        loadProject->OnNewRound = root["OnNewRound"].as<std::vector<std::string>>();

        loadProject->Refresh();
        this->LogSucc("成功从文件加载项目：" + loadProjectName);

        //添加进项目组
        this->projects[loadProjectName] = std::move(loadProject);
        return true;
    }
    catch (const YAML::Exception& e) {
        this->LogError("在加载项目时出现问题：" + std::string(e.what()));
        return false;
    }
}