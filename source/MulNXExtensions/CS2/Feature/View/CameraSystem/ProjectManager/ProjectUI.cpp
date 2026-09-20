#include"ProjectManager.hpp"
#include <MulNX/Base/UI/UI.hpp>

bool ProjectManager::MenuProject() {
    std::shared_lock lock(this->smutex);
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
void ProjectManager::UINodeFunc() {
    if (!this->showWindow.load(std::memory_order_acquire))return;
    //项目调试窗口
    this->Project_DebugWindow();
    if (!this->OpenProjectKCPackDebugWindow)return;
    auto buffer = this->bufKCPack.load();
    auto p = buffer.DebugWindow("运镜包快捷切换按键绑定", this->OpenProjectKCPackDebugWindow);
    if (!p.first.has_value())return;
    this->bufKCPack = *p.first;
    if (!p.second)return;
    this->OpenProjectKCPackDebugWindow.store(false, std::memory_order_release);
    this->ControllingProject->KCPack = p.first.value();//更新绑键
}
void ProjectManager::Project_DebugWindow() {
    auto w = MulNX::UI::RAIIWindow(I18n("camsys.proj.debug_window").c_str(), this->showWindow);
    if (!w || !w.ShouldDraw())return;
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
        this->bufKCPack = this->ControllingProject->KCPack;
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