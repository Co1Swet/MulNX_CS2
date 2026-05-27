#include "DemoSystem.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/CS2/CSController/CSController.hpp>
#include <MulNXExtensions/CS2/TimeController/TimeController.hpp>

bool DemoSystem::Window(MulNX::UINode* node) {
    auto w = MulNX::UI::RAIIWindow("Demo", this->showWindow);
    if (!w) return true;
    {
        std::unique_lock lock(this->smutex);

        if (ImGui::Button(I18n("demo.refresh").c_str())) {
            this->ISys().PublishAsync("Demo/Refresh"_hash);
        }

        if (this->demoFiles.empty()) {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", I18n("demo.empty").c_str());
        }
        else {
            // 可复制的列表控件标识
            ImGui::BeginChild("DemoFileList", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 4), true);

            int index = 0;
            // 遍历文件集合（set 自动按路径排序）
            for (auto it = this->demoFiles.begin(); it != this->demoFiles.end(); ) {
                const auto& filePath = *it;
                std::string fullPath = filePath.string();

                bool anylized = false;
                if (std::filesystem::exists(filePath.parent_path() / (filePath.stem().string() + ".json"))) {
                    anylized = true;
                }

                // 用 Selectable 展示条目，支持高亮
                bool isSelected = (this->selectedDemoIndex == index);
                std::string fileName = filePath.filename().string() + "      " + (anylized ? "已分析" : "未分析");
                if (ImGui::Selectable(fileName.c_str(), isSelected, ImGuiSelectableFlags_AllowDoubleClick)) {
                    this->selectedDemoIndex = index;
                }
                ImGui::SameLine();

                // 右键菜单（或在 Selectable 上悬浮右键）
                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem(I18n("demo.play").c_str())) {
                        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/Play"_hash);
                        rp->str1 = fullPath;
                        this->ISys().PublishAsync(std::move(msg));
                    }
                    if (ImGui::MenuItem(I18n("demo.analyze").c_str())) {
                        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/Analyze"_hash);
                        rp->str1 = fullPath;
                        this->ISys().PublishAsync(std::move(msg));
                    }
                    if (ImGui::MenuItem(I18n("demo.load").c_str())) {
                        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/JSON/Load"_hash);
                        rp->str1 = filePath.stem().string();
                        this->ISys().PublishAsync(std::move(msg));
                    }
                    if (ImGui::MenuItem(I18n("demo.copy_path").c_str())) {
                        ImGui::SetClipboardText(fullPath.c_str());
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem(I18n("demo.remove").c_str())) {
                        // 从集合中移除，并安全递增迭代器
                        it = this->demoFiles.erase(it);
                        // 调整选中索引
                        if (this->selectedDemoIndex >= static_cast<int>(this->demoFiles.size()))
                            this->selectedDemoIndex = std::max(0, static_cast<int>(this->demoFiles.size()) - 1);
                        ImGui::EndPopup();
                        continue; // 跳过 ++it
                    }
                    ImGui::EndPopup();
                }

                ++it;
                ++index;

                ImGui::NewLine();
            }

            ImGui::EndChild();

            // 底部操作按钮
            if (ImGui::Button(I18n("demo.clear_all").c_str())) {
                this->demoFiles.clear();
                this->selectedDemoIndex = -1;
            }
            ImGui::SameLine();
            if (ImGui::Button(I18n("demo.play_selected").c_str())) {
                if (this->selectedDemoIndex >= 0 &&
                    this->selectedDemoIndex < static_cast<int>(this->demoFiles.size())) {
                    auto iter = this->demoFiles.begin();
                    std::advance(iter, this->selectedDemoIndex);
                    auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/Play"_hash);
                    rp->str1 = iter->string();
                    this->ISys().PublishAsync(std::move(msg));
                }
            }
        }
    }
    ImGui::Separator();

    ImGui::Text(I18n("demo.status.is_playing", this->CS2Time()->IsPlayingDemo()).c_str());
    ImGui::Text(I18n("demo.status.is_pausing", this->CS2Time()->IsDemoPaused()).c_str());

    ImGui::Separator();

    node->CallUINode("RecordTaskMaker");
    node->CallUINode("RecordTaskConfiger");
    node->CallUINode("DemoHelper");
    node->CallUINode("DemoRecorder");
    return true;
}

bool DemoSystem::Init() {
    this->pathDemos = this->ISys().PathManager()->PathGetForShared("Demos");

    std::function<void(CCommand*)> f;

    this->ISys()
        .SubscribeAsync("Demo/Play")
        .SubscribeAsync("Demo/Refresh")
        .SubscribeAsync("Window/Drag/FileDrop")
        .SubscribeSync("Hook/RegisterConCommand/RegisterOurCmd", [this](MulNX::Message& msg) {
        this->CS2()->RegisterCS2Cmd("cl_MulNX", "this is MulNX Cmd", [this](CCommand* a) {
            MessageBoxW(NULL, L"test", L"test", MB_OK);
            return;
            });
            });

    this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {
        return this->Window(node);
        });

    this->ISys().SendTask("Update", "DemoSys", [this]() {
        this->Update();
        return true;
        });

    this->ISys().PublishAsync("Demo/Refresh"_hash);

    return true;
}

void DemoSystem::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Window/Drag/FileDrop"_hash: {
        auto& path = msg.asp.get<MulNX::NetExt>()->str1;
        std::filesystem::path file = path;
        auto ext = file.extension();
        if (ext != ".dem")break;
        try {
            std::filesystem::copy(file, this->pathDemos / file.filename(), std::filesystem::copy_options::overwrite_existing);
        }
        catch (const std::filesystem::filesystem_error& e) {
            this->ISys().LogError(I18n("demo.copy_failed", e.what()).c_str());
        }
        this->ISys().PublishAsync("Demo/Refresh"_hash);
        break;
    }
    case "Demo/Refresh"_hash: {
        std::unique_lock lock(this->smutex);
        this->demoFiles.clear();
        for (const auto& entry : std::filesystem::directory_iterator(this->pathDemos)) {
            if (entry.is_regular_file() && entry.path().extension() == ".dem") {
                this->demoFiles.insert(entry.path());
            }
        }
        break;
    }
    case "Demo/Play"_hash: {
        auto& path = msg.asp.get<MulNX::NetExt>()->str1;
        this->ISys().AsyncCommand(std::format("playdemo \"{}\"", path));
        break;
    }
    }

}
