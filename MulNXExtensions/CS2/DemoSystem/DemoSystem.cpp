#include "DemoSystem.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/CS2/CSController/CSController.hpp>

bool DemoSystem::Window(MulNX::UINode* node) {
    auto w = MulNX::UI::RAIIWindow("Demo", this->ShowWindow);
    if (!w) return true;
    {
        std::unique_lock lock(this->smutex);

        if (this->DemoFiles.empty()) {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", I18n("demo.empty").c_str());
        }
        else {
            // 可复制的列表控件标识
            ImGui::BeginChild("DemoFileList", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 4), true);

            int index = 0;
            // 遍历文件集合（set 自动按路径排序）
            for (auto it = this->DemoFiles.begin(); it != this->DemoFiles.end(); ) {
                const auto& filePath = *it;
                std::string fileName = filePath.filename().string(); // 只显示文件名
                std::string fullPath = filePath.string();

                // 用 Selectable 展示条目，支持高亮
                bool isSelected = (this->selectedDemoIndex == index);
                if (ImGui::Selectable(fileName.c_str(), isSelected, ImGuiSelectableFlags_AllowDoubleClick)) {
                    this->selectedDemoIndex = index;
                    // 双击直接播放
                    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                        std::string cmd = std::format("playdemo \"{}\"", fullPath);
                        this->ISys().AsyncCommand(std::move(cmd));
                    }
                }

                // 右键菜单（或在 Selectable 上悬浮右键）
                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem(I18n("demo.play").c_str())) {
                        std::string cmd = std::format("playdemo \"{}\"", fullPath);
                        this->ISys().AsyncCommand(std::move(cmd));
                    }
                    if (ImGui::MenuItem(I18n("demo.copy_path").c_str())) {
                        ImGui::SetClipboardText(fullPath.c_str());
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem(I18n("demo.remove").c_str())) {
                        // 从集合中移除，并安全递增迭代器
                        it = this->DemoFiles.erase(it);
                        // 调整选中索引
                        if (this->selectedDemoIndex >= static_cast<int>(this->DemoFiles.size()))
                            this->selectedDemoIndex = std::max(0, static_cast<int>(this->DemoFiles.size()) - 1);
                        ImGui::EndPopup();
                        continue; // 跳过 ++it
                    }
                    ImGui::EndPopup();
                }

                ++it;
                ++index;
            }

            ImGui::EndChild();

            // 底部操作按钮
            if (ImGui::Button(I18n("demo.clear_all").c_str())) {
                this->DemoFiles.clear();
                this->selectedDemoIndex = -1;
            }
            ImGui::SameLine();
            if (ImGui::Button(I18n("demo.play_selected").c_str())) {
                if (this->selectedDemoIndex >= 0 &&
                    this->selectedDemoIndex < static_cast<int>(this->DemoFiles.size())) {
                    auto iter = this->DemoFiles.begin();
                    std::advance(iter, this->selectedDemoIndex);
                    std::string cmd = std::format("playdemo \"{}\"", iter->string());
                    this->ISys().AsyncCommand(std::move(cmd));
                }
            }
        }
    }
    ImGui::Separator();

    ImGui::Text(I18n("demo.status.is_playing", this->CS2()->IsPlayingDemo()).c_str());
    ImGui::Text(I18n("demo.status.is_pausing", this->CS2()->IsDemoPaused()).c_str());

    ImGui::Separator();
    node->CallUINode("DemoHelper");
    return true;
}

bool DemoSystem::Init() {
    this->ISys()
        .SubscribeAsync("Window/Drag/FileDrop");

    this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {
        return this->Window(node);
        });
    
    this->SendTask("MulNXMain", [this]() {
        this->EntryProcessMsg();
        return true;
        });

    return true;
}

void DemoSystem::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Window/Drag/FileDrop"_hash: {
        auto& path = msg.asp.get<MulNX::NetExt>()->str1;
        std::filesystem::path file = path;
        auto ext = file.extension();
        if (ext != ".dem")break;
        this->DemoFiles.insert(std::move(file));
        //this->ISys().AsyncCommand(std::format("playdemo \"{}\"", path));
        break;
    }
    }
    
}
