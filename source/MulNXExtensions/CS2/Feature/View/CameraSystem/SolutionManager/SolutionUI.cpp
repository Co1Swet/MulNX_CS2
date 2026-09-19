#include "SolutionManager.hpp"

bool SolutionManager::MenuSolution() {
    std::shared_lock lock(this->smutex);
    ImGui::Separator();
    // 解决方案总设置
    if (ImGui::CollapsingHeader(I18n("camsys.sol.settings").c_str())) {
        ImGui::Checkbox(I18n("camsys.sol.shortcut_enable").c_str(), &this->Config.SolutionShortcutEnable);
        ImGui::Checkbox(I18n("camsys.sol.playing_draw").c_str(), &this->Config.PlayingDraw);
        ImGui::Checkbox(I18n("camsys.sol.playing_override").c_str(), &this->Config.PlayingOverride);
    }
    // 创建解决方案
    if (ImGui::CollapsingHeader(I18n("camsys.sol.create").c_str())) {
        ImGui::Text(I18n("camsys.sol.new_name").c_str());
        ImGui::SameLine();
        static std::string CreateSolutionName = "";
        ImGui::InputText("##新解决方案名", &CreateSolutionName);
        ImGui::SameLine();
        // 创建成功则清空输入框
        if (ImGui::Button(I18n("camsys.sol.create_btn").c_str())) {
            if (CreateSolutionName.empty()) {
                this->LogError(I18n("result.error_empty_name").c_str());
                return true;
            }
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CameraSystem/Solution/Create"_hash);
            rp->str1 = std::move(CreateSolutionName);
            this->PublishAsync(std::move(msg));
            CreateSolutionName.clear();
        }
    }
    // 展示修改解决方案
    if (ImGui::CollapsingHeader(I18n("camsys.sol.list").c_str())) {
        for (const auto& [name, solution] : this->solutions) {
            this->Solution_ShowInLine(solution.get());
        }
    }

    return true;
}
void SolutionManager::Solution_ShowInLine(const Solution* solution)const {
    ImGui::Text(I18n("camsys.sol.name_label").c_str());
    ImGui::SameLine();
    if (ImGui::Selectable(solution->name.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
        if (ImGui::IsMouseDoubleClicked(0)) {
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CamMacro/OpenDebug"_hash);
            rp->str1 = solution->GetName();
            this->PublishAsync(std::move(msg));
        }
    }
    if (ImGui::BeginPopupContextItem(("右键菜单" + solution->name).c_str())) {
        if (ImGui::MenuItem(I18n("text.copy_name").c_str())) {
            ImGui::SetClipboardText(solution->name.c_str());
        }
        if (ImGui::MenuItem(I18n("text.save").c_str())) {
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CamMacro/SaveOne"_hash);
            rp->str1 = solution->GetName();
            this->PublishAsync(std::move(msg));
        }
        if (ImGui::MenuItem(I18n("text.delete").c_str())) {
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CameraSystem/Solution/Delete"_hash);
            rp->str1 = solution->name;
            this->PublishAsync(std::move(msg));
        }
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    ImGui::Text(I18n("camsys.sol.element_count_duration",
        solution->elements.size(),
        solution->totalDurationTime
    ).c_str());
}
bool SolutionManager::UINodeFunc() {
    std::shared_lock lock(this->smutex);
    if (!this->showWindow.load(std::memory_order_acquire))return true;
    
    if (this->CurrentSolution) {
        this->Solution_DebugWindow(this->CurrentSolution);
    }
    else {
        this->OpenSolutionKCPackDebugWindow = false;
    }
    if (!this->OpenSolutionKCPackDebugWindow)return true;
    if (this->Buffer_KCPack.DebugWindow(this->OpenSolutionKCPackDebugWindow)) {
        this->OpenSolutionKCPackDebugWindow.store(false, std::memory_order_release);
        if (!this->Buffer_KCPack.Usable) {
            this->LogError("当前按键绑定不可用，无法使用这个绑键播放解决方案！");
        }
        else {
            this->CurrentSolution->KCPack = this->Buffer_KCPack;//更新绑键
        }
    }
    return true;
}

void SolutionManager::Solution_DebugWindow(const Solution* pMacro)const {
    auto w = MulNX::UI::RAIIWindow("运镜宏调试", this->showWindow);
    if (!w || !w.ShouldDraw())return;
    // 检查当前是否操作解决方案
    if (!this->CurrentSolution) {
        ImGui::Text("当前未选择任何宏");
        return;
    }

    ImGui::Text(I18n("camsys.sol.current_info",
        this->CurrentSolution->name,
        this->CurrentSolution->elements.size(),
        this->CurrentSolution->totalDurationTime,
        PlaybackModeToString(this->CurrentSolution->playmode)
    ).c_str());
    if (ImGui::Button(I18n("camsys.sol.switch_to_activation").c_str())) {
        this->CurrentSolution->playmode = PlaybackMode::Activation;
    }
    ImGui::SameLine();
    if (ImGui::Button(I18n("camsys.sol.switch_to_orchestration").c_str())) {
        this->CurrentSolution->playmode = PlaybackMode::Orchestration;
    }
    if (ImGui::Button(I18n("camsys.sol.enable_current").c_str())) {
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CameraSystem/Solution/Play"_hash);
        rp->str1 = this->CurrentSolution->name;
        this->PublishAsync(std::move(msg));
    }
    ImGui::SameLine();
    if (ImGui::Button("编辑宏绑定")) {
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CamMacro/DebugKCP"_hash);
        rp->str1 = pMacro->GetName();
        this->PublishAsync(std::move(msg));
    }
    ImGui::Separator();

    static std::string newCampathName{};
    ImGui::InputText("目标运镜轨道名", &newCampathName);
    if (ImGui::Button("为当前宏添加运镜轨道")) {
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CamMacro/AddCampath"_hash);
        rp->str1 = pMacro->GetName();
        rp->str2 = std::move(newCampathName);
        newCampathName.clear();
        this->PublishAsync(std::move(msg));
    }
    ImGui::SameLine();
    if (ImGui::Button(I18n("text.clear").c_str())) {
        this->CurrentSolution->Clear();
        this->LogSucc("成功清空解决方案所有元素");
    }

    ImGui::Separator();

    static int IndexForReset = 0;
    static int PreIndex = -1;
    ImGui::SliderInt(I18n("camsys.sol.adjust_element_index").c_str(), &IndexForReset, 0, this->CurrentSolution->elements.size() - 1);
    if (this->CurrentSolution->elements.empty())return;
    // std::shared_ptr<FreeCameraPath> element = this->CurrentSolution->elements.at(IndexForReset).Element;
    // if (element) {
    //     const float& Offset = this->CurrentSolution->elements.at(IndexForReset).Offset;
    //     ImGui::Text(I18n("camsys.sol.element_info",
    //         IndexForReset, element->GetName(), element->DurationTime, Offset).c_str());
    //     ImGui::Separator();
    //     static float tempOffset{};
    //     if (IndexForReset != PreIndex) {
    //         tempOffset = Offset;
    //     }
    //     ImGui::SliderFloat(I18n("camsys.sol.offset_time").c_str(), &tempOffset, 0, 100000);
    //     if (ImGui::Button(I18n("text.confirm_modify").c_str())) {
    //         this->CurrentSolution->RemoveElementAt(IndexForReset);
    //         //this->CurrentSolution->AddElement(element, tempOffset);
    //     }
    //     if (ImGui::Button(I18n("text.remove").c_str())) {
    //         this->CurrentSolution->RemoveElementAt(IndexForReset);
    //     }
    // }
    // PreIndex = IndexForReset;
}