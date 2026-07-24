#include"ProjectManager.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <CameraSystem/ElementManager/ElementManager.hpp>
#include <CameraSystem/SolutionManager/SolutionManager.hpp>

bool ProjectManager::MenuProject() {
    // 项目总设置
    if (ImGui::CollapsingHeader(I18n("camsys.proj.settings").c_str())) {
        ImGui::Checkbox(I18n("camsys.proj.shortcut_enable").c_str(), &this->Config.ProjectShortcutEnable);
    }
    // 创建项目
    if (ImGui::CollapsingHeader(I18n("camsys.proj.create").c_str())) {
        static std::string CreateProjectName = "";
        ImGui::Text(I18n("camsys.proj.new_name").c_str());
        ImGui::SameLine();
        ImGui::InputText("##CreateProject", &CreateProjectName);
        ImGui::SameLine();
        if (ImGui::Button(I18n("text.create").c_str())) {
            if (CreateProjectName.empty()) {
                this->LogError(I18n("result.error_empty_name"));
                return true;
            }
            if (this->Project_Create(CreateProjectName)) {
                CreateProjectName.clear();
            }
        }

    }
    // 展示修改项目
    if (ImGui::CollapsingHeader(I18n("camsys.proj.list").c_str())) {
        for (const auto& [name, project] : this->projects) {
            ImGui::Text(I18n("camsys.proj.name_label").c_str());
            ImGui::SameLine();
            if (ImGui::Button(project->Name.c_str())) {
                this->ControllingProject = project;
                this->showWindow.store(true, std::memory_order_release);
            }
        }
    }
    return true;
}
bool ProjectManager::UINodeFunc() {
    if (this->showWindow.load(std::memory_order_acquire)) {
        //项目调试窗口
        this->Project_DebugWindow();
        if (this->OpenProjectKCPackDebugWindow) {
            //项目按键绑定调试窗口
            if(this->Buffer_KCPack.DebugWindow(this->OpenProjectKCPackDebugWindow)) {
                this->ControllingProject->KCPack = this->Buffer_KCPack;//修改按键绑定
            }
        }
    }
    return true;
}
void ProjectManager::Project_DebugWindow() {
    auto w = MulNX::UI::RAIIWindow(I18n("camsys.proj.debug_window").c_str(), this->showWindow);
    if (!w)return;
    // 检查当前操作项目
    if (!this->ControllingProject) {
        ImGui::Text(I18n("camsys.proj.no_selected").c_str());
        return;
    }
    ImGui::Text(I18n("camsys.proj.current_label").c_str());
    ImGui::SameLine();
    ImGui::Text(this->ControllingProject->Name.c_str());
    if (ImGui::Button(I18n("camsys.proj.switch_to_current").c_str())) {
        this->Project_Apply(this->ControllingProject);
    }
    if (ImGui::Button(I18n("camsys.proj.unload_current").c_str())) {
        this->Project_Delete(this->ControllingProject->Name);
        this->showWindow.store(false, std::memory_order_release);
        return;
    }
    if (ImGui::Button(I18n("camsys.proj.modify_keybind").c_str())) {
        this->Buffer_KCPack = this->ControllingProject->KCPack;
        this->OpenProjectKCPackDebugWindow = true;
    }
    ImGui::Separator();
    ImGui::Separator();
    if (ImGui::TreeNode(I18n("camsys.proj.enter_new_round").c_str())) {
        if (!this->ControllingProject->OnNewRound.empty()) {
            for (const std::string SolutionName : this->ControllingProject->OnNewRound) {
                ImGui::Text(SolutionName.c_str());
                ImGui::SameLine();
                if (ImGui::Button((I18n("camsys.proj.delete_on_new_round") + SolutionName).c_str())) {
                    auto it = std::find(this->ControllingProject->OnNewRound.begin(), this->ControllingProject->OnNewRound.end(), SolutionName);
                    if (it != this->ControllingProject->OnNewRound.end()) {
                        this->ControllingProject->OnNewRound.erase(it);
                    }
                }
            }
        }
        else {
            ImGui::Text(I18n("text.empty").c_str());
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNode(I18n("camsys.proj.round_end").c_str())) {
        if (!this->ControllingProject->OnRoundEnd.empty()) {
            for (const std::string SolutionName : this->ControllingProject->OnRoundEnd) {
                ImGui::Text(SolutionName.c_str());
            }
        }
        else {
            ImGui::Text(I18n("text.empty").c_str());
        }
        ImGui::TreePop();
    }
}

bool ProjectManager::Init() {
    this->EManager = this->Core->ModuleManager()->FindModule<ElementManager>("ElementManager");
    this->SManager = this->Core->ModuleManager()->FindModule<SolutionManager>("SolutionManager");
    this->pIPCer = this->Core->ModuleManager()->FindModule<MulNX::IPCer>("IPCer");

    this->SendUIRoot(this->GetName(), [this](auto&&...) {return this->UINodeFunc();});

    auto* PathManager = this->Path();
    if (PathManager->CreateKey("CurrentProject", {},
        [this](MulNX::PathManager* PathManager)->bool {
            auto NewProjectPath = PathManager->PathGetFromKey("CurrentProject");
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
        })) {
        PathManager->KeyBindDynamic("CurrentProject", "CurrentWorkspace");
    }

    return true;
}
void ProjectManager::HandleUpdate() {
    if (!this->Config.ProjectShortcutEnable)return;
    for (const auto& [name, project] : this->projects) {
        if (this->pInputSystem->CheckWithPack(project->KCPack)) {
            this->Project_Apply(project);
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
    if (this->projects.find(name)!=this->projects.end()) {
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
    ////获取元素名称列表
    //this->ActiveProject->ElementNames = std::move(this->EManager->Element_GetNames());
    ////获取解决方案名称列表
    //this->ActiveProject->SolutionNames = std::move(this->SManager->Solution_GetNames());
    this->ActiveProject->Refresh();
    //刷新完毕
    return true;
}
bool ProjectManager::Project_Save() {
    if (!this->Project_Refresh()) {
        return false;
    }
    //保存所有元素和解决方案到磁盘
    this->EManager->Element_SaveAll();
    this->SManager->Solution_SaveAll();
    //保存项目到磁盘
    std::filesystem::path Path = this->Path()->PathGetFromKey("CurrentWorkspace") / this->ActiveProject->Name;
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
    //清空旧解决方案，防止冲突
    this->SManager->Solution_ClearAll();
    //清空旧元素，防止冲突
    this->EManager->Element_ClearAll();
    if (!this->Path()->KeySetCurrent("CurrentProject", Project->Name)) {
        this->LogError("尝试切换到项目时出现问题，设置项目文件夹路径失败！");
        return false;
    }
    //获取元素文件夹路径
    std::filesystem::path ElementsPath = this->Path()->PathGetFromKey("Elements");
    std::vector<std::string>Elements = this->pIPCer->GetFileNamesByPath(ElementsPath);
    //遍历加载元素
    for (const std::string& Element : Elements) {
        this->EManager->Element_Load(ElementsPath / Element);
    }
    //获取解决方案文件夹路径
    std::filesystem::path SolutionsPath = this->Path()->PathGetFromKey("Solutions");
    std::vector<std::string>Solutions = this->pIPCer->GetFileNamesByPath(SolutionsPath);
    //遍历加载解决方案
    for (const std::string& Solution : Solutions) {
        this->SManager->Solution_Load(SolutionsPath / Solution);
    }
    this->ActiveProject = Project;
    this->LogSucc("已切换至项目" + Project->Name);
    this->LogSucc("尝试加载元素总数：" + std::to_string(Elements.size()));
    this->LogSucc("尝试加载解决方案总数：" + std::to_string(Solutions.size()));
    this->LogSucc("成功加载元素总数：" + std::to_string(this->EManager->elements.size()));
    this->LogSucc("成功加载解决方案总数：" + std::to_string(this->SManager->solutions.size()));
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
        if (this->projects.find(loadProjectName)!=this->projects.end()) {
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
bool ProjectManager::Playing_AutoCall(const MulNX::Message& Msg) {
    this->LogInfo("项目管理器正在处理消息！");
    if (!this->ActiveProject) {
        this->LogWarning("无活跃项目，无法执行自动操作");
        return false;
    }
    switch (Msg.type) {
    case "Game/NewRound"_hash: {
        const std::vector<std::string>& OnNewRound = this->ActiveProject->OnNewRound;
        if (OnNewRound.empty()) {
            this->LogWarning("无新回合解决方案可尝试调用");
            return false;
        }
        int temp = rand() % OnNewRound.size();
        auto [msg,rp] = MulNX::Message::Create<MulNX::NetExt>("CameraSystem/Solution/Play"_hash);
        rp->str1 = OnNewRound[temp];
        this->PublishAsync(std::move(msg));
        return true;
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
    return false;
}