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
    if (ImGui::CollapsingHeader("创建新宏")) {
        ImGui::SameLine();
        static std::string createMacroName = "";
        ImGui::InputText("新宏名", &createMacroName);
        ImGui::SameLine();
        // 创建成功则清空输入框
        if (ImGui::Button("确认创建新宏")) {
            if (createMacroName.empty()) {
                this->LogError("宏名不能是空的！");
                return true;
            }
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CameraSystem/Solution/Create"_hash);
            rp->str1 = std::move(createMacroName);
            this->PublishAsync(std::move(msg));
            createMacroName.clear();
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
    if (ImGui::Selectable(solution->GetName().c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
        if (ImGui::IsMouseDoubleClicked(0)) {
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CamMacro/OpenDebug"_hash);
            rp->str1 = solution->GetName();
            this->PublishAsync(std::move(msg));
        }
    }
    if (ImGui::BeginPopupContextItem(("右键菜单" + solution->GetName()).c_str())) {
        if (ImGui::MenuItem(I18n("text.copy_name").c_str())) {
            ImGui::SetClipboardText(solution->GetName().c_str());
        }
        if (ImGui::MenuItem(I18n("text.save").c_str())) {
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CamMacro/SaveOne"_hash);
            rp->str1 = solution->GetName();
            this->PublishAsync(std::move(msg));
        }
        if (ImGui::MenuItem(I18n("text.delete").c_str())) {
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CameraSystem/Solution/Delete"_hash);
            rp->str1 = solution->GetName();
            this->PublishAsync(std::move(msg));
        }
        ImGui::EndPopup();
    }
}
void SolutionManager::UINodeFunc()const {
    std::shared_lock lock(this->smutex);
    if (!this->showWindow.load(std::memory_order_acquire))return;
    if (!this->CurrentSolution) {
        this->OpenSolutionKCPackDebugWindow = false;
        return;
    }
    this->Solution_DebugWindow(this->CurrentSolution);
    if (!this->OpenSolutionKCPackDebugWindow)return;
    auto buffer = this->bufKCPack.load();
    auto p = buffer.DebugWindow("宏按键绑定", this->OpenSolutionKCPackDebugWindow);
    if (!p.first.has_value())return;
    this->bufKCPack = *p.first;
    if (!p.second)return;
    this->OpenSolutionKCPackDebugWindow.store(false, std::memory_order_release);
    this->CurrentSolution->SetKeyCheckPack(*p.first);//更新绑键
}

void SolutionManager::Solution_DebugWindow(const Solution* pMacro) const {
    auto w = MulNX::UI::RAIIWindow("运镜宏调试", this->showWindow);
    if (!w || !w.ShouldDraw()) return;

    ImGui::Text(std::format("宏名称：{}", pMacro->GetName()).c_str());

    if (ImGui::Button(I18n("camsys.sol.enable_current").c_str())) {
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CameraSystem/Solution/Play"_hash);
        rp->str1 = pMacro->GetName();
        this->PublishAsync(std::move(msg));
    }
    ImGui::SameLine();
    if (ImGui::Button("编辑宏绑定")) {
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CamMacro/DebugKCP"_hash);
        rp->str1 = pMacro->GetName();
        this->PublishAsync(std::move(msg));
    }

    ImGui::Separator();

    static std::string newCampathName;
    ImGui::InputText("目标运镜轨道名", &newCampathName);

    if (ImGui::Button("为当前宏添加运镜轨道")) {
        if (!newCampathName.empty()) {
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CamMacro/AddCampath"_hash);
            auto&& [offset] = msg.Access<float>();
            offset = 0.0f;
            rp->str1 = pMacro->GetName();
            rp->str2 = newCampathName;
            newCampathName.clear();
            this->PublishAsync(std::move(msg));
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("清除宏的所有运镜")) {
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CamMacro/ClearOne"_hash);
        rp->str1 = pMacro->GetName();
        this->PublishAsync(std::move(msg));
    }

    ImGui::Separator();

    const auto& vec = pMacro->GetVec();

    static int selectedIndex = -1;
    static int lastSelected = -2;
    static float tempOffset = 0.0f;

    if (vec.empty()) {
        selectedIndex = -1;
        lastSelected = -2;
        ImGui::TextDisabled("当前宏没有绑定任何运镜轨道");
        return;
    }

    if (selectedIndex >= static_cast<int>(vec.size())) {
        selectedIndex = static_cast<int>(vec.size()) - 1;
    }
    if (selectedIndex < -1) {
        selectedIndex = -1;
    }

    ImGui::SeparatorText("运镜轨道列表");

    std::string listBoxId = std::format("##campath_list_{}", pMacro->GetName());
    ImGui::BeginListBox(listBoxId.c_str(), ImVec2(-FLT_MIN, 8 * ImGui::GetTextLineHeightWithSpacing()));

    for (int i = 0; i < static_cast<int>(vec.size()); ++i) {
        const auto& item = vec[i];
        const bool isSelected = (selectedIndex == i);

        std::string label = std::format(
            "[{}] {}    偏移: {}",
            i, item.campathName, item.offset
        );

        if (ImGui::Selectable(label.c_str(), isSelected)) {
            selectedIndex = i;
        }
    }

    ImGui::EndListBox();

    if (selectedIndex >= 0 && selectedIndex < static_cast<int>(vec.size())) {
        const auto& item = vec[selectedIndex];

        ImGui::SeparatorText("编辑选中轨道");
        ImGui::Text("轨道名：%s", item.campathName.c_str());
        ImGui::Text("当前偏移：%.2f", item.offset);

        if (selectedIndex != lastSelected) {
            tempOffset = item.offset;
            lastSelected = selectedIndex;
        }

        ImGui::SliderFloat("运镜时间偏移", &tempOffset, 0.0f, 100000.0f);

        if (ImGui::Button("确认调整")) {
            auto [remove, pRemove] = MulNX::Message::Create<MulNX::NetExt>("CamMacro/RemoveCampath"_hash);
            pRemove->str1 = pMacro->GetName();
            pRemove->str2 = item.campathName;
            this->PublishAsync(std::move(remove));

            auto [add, pAdd] = MulNX::Message::Create<MulNX::NetExt>("CamMacro/AddCampath"_hash);
            auto&& [addOffset] = add.Access<float>();
            addOffset = tempOffset;
            pAdd->str1 = pMacro->GetName();
            pAdd->str2 = item.campathName;
            this->PublishAsync(std::move(add));
        }

        ImGui::SameLine();

        if (ImGui::Button("从宏中移除该运镜")) {
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CamMacro/RemoveCampath"_hash);
            rp->str1 = pMacro->GetName();
            rp->str2 = item.campathName;
            this->PublishAsync(std::move(msg));
        }
    }
}