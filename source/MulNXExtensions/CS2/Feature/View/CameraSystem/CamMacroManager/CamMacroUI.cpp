#include "CamMacroManager.hpp"

bool CamMacroManager::Menu() {
    std::shared_lock lock(this->smutex);
    if (ImGui::CollapsingHeader("运镜宏总控")) {
        ImGui::Checkbox("启用运镜宏快捷键触发", &this->shortcutEnable);
    }
    if (ImGui::CollapsingHeader("创建新宏")) {
        static std::string createMacroName = "";
        ImGui::InputText("新宏名", &createMacroName);
        ImGui::SameLine();
        if (ImGui::Button("确认创建新宏")) {
            if (createMacroName.empty()) {
                this->LogError("宏名不能是空的！");
                return true;
            }
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CamMacro/Create"_hash);
            rp->str1 = std::move(createMacroName);
            this->PublishAsync(std::move(msg));
            createMacroName.clear();
        }
    }
    if (ImGui::CollapsingHeader("运镜宏列表")) {
        for (const auto& [name, pCamMacro] : this->camMacros) {
            this->CamMacroShowOneLine(pCamMacro.get());
        }
    }

    return true;
}
void CamMacroManager::CamMacroShowOneLine(const CamMacro* pCamMacro)const {
    if (ImGui::Selectable(pCamMacro->GetName().c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
        if (ImGui::IsMouseDoubleClicked(0)) {
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CamMacro/OpenDebug"_hash);
            rp->str1 = pCamMacro->GetName();
            this->PublishAsync(std::move(msg));
        }
    }
    if (ImGui::BeginPopupContextItem(pCamMacro->GetName().c_str())) {
        if (ImGui::MenuItem("复制运镜宏名到剪贴板")) {
            ImGui::SetClipboardText(pCamMacro->GetName().c_str());
        }
        if (ImGui::MenuItem("保存运镜宏到磁盘")) {
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CamMacro/SaveOne"_hash);
            rp->str1 = pCamMacro->GetName();
            this->PublishAsync(std::move(msg));
        }
        if (ImGui::MenuItem("删除运镜宏")) {
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CamMacro/Delete"_hash);
            rp->str1 = pCamMacro->GetName();
            this->PublishAsync(std::move(msg));
        }
        ImGui::EndPopup();
    }
}
void CamMacroManager::UI()const {
    std::shared_lock lock(this->smutex);
    if (!this->showWindow.load(std::memory_order_acquire))return;
    if (!this->pOperatingMacro) {
        this->bKCPWindow = false;
        return;
    }
    this->CamMacroDebugWindow(this->pOperatingMacro);
    if (!this->bKCPWindow)return;
    auto buffer = this->bufKCPack.load();
    auto p = buffer.DebugWindow("宏按键绑定", this->bKCPWindow);
    if (!p.first.has_value())return;
    this->bufKCPack = *p.first;
    if (!p.second)return;
    this->bKCPWindow.store(false, std::memory_order_release);
    this->pOperatingMacro->SetKeyCheckPack(*p.first);//更新绑键
}

void CamMacroManager::CamMacroDebugWindow(const CamMacro* pMacro) const {
    auto w = MulNX::UI::RAIIWindow("运镜宏调试", this->showWindow);
    if (!w || !w.ShouldDraw()) return;

    ImGui::Text(std::format("宏名称：{}", pMacro->GetName()).c_str());

    if (ImGui::Button("使用运镜宏")) {
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CamMacro/Play"_hash);
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