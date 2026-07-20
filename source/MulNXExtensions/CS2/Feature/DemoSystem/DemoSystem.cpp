#include "DemoSystem.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <Intro/HookConsole/HookConsole.hpp>
#include <Buildup/TimeController/TimeController.hpp>

bool DemoSystem::Window(MulNX::UICoordinator* uico) {
    auto w = MulNX::UI::RAIIWindow("Demo", this->showWindow);
    if (!w) return true;

    uico->CallbackCall("UI.Demos"_hash, nullptr);
    uico->CallUINode("DemoRecorder");

    std::unique_lock lock(this->smutex);

    if (ImGui::Button(I18n("demo.refresh").c_str())) {
        this->PublishAsync("Demo/Refresh"_hash);
    }

    if (this->demoFiles.empty()) {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", I18n("demo.empty").c_str());
    }

    // 可复制的列表控件标识
    ImGui::BeginChild("DemoFileList", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 4), true);

    int index = 0;
    // 遍历文件集合（set 自动按路径排序）
    for (auto it = this->demoFiles.begin(); it != this->demoFiles.end(); ) {
        const auto& filePath = *it;
        std::string fullPath = filePath.string();

        bool anylized = false;
        if (std::filesystem::exists(this->dirData / (filePath.stem().string() + ".json"))) {
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
                this->PublishAsync(std::move(msg));
            }
            if (ImGui::MenuItem(I18n("demo.analyze").c_str())) {
                auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/Analyze"_hash);
                rp->str1 = fullPath;
                this->PublishAsync(std::move(msg));
            }
            if (ImGui::MenuItem(I18n("demo.load").c_str())) {
                auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/JSON/Load"_hash);
                rp->str1 = filePath.stem().string();
                this->PublishAsync(std::move(msg));
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
            this->PublishAsync(std::move(msg));
        }
    }

    ImGui::Separator();

    ImGui::Text(I18n("demo.status.is_playing", this->CS2Time->IsPlayingDemo()).c_str());
    ImGui::Text(I18n("demo.status.is_pausing", this->CS2Time->IsDemoPaused()).c_str());

    ImGui::Separator();

    return true;
}

bool DemoSystem::Init() {
    this->dirData = this->Path()->PathGetForShared("Data");

    (*this)
        .SubscribeAsync("Demo/Play")
        .SubscribeAsync("Demo/Refresh")
        .SubscribeAsync("Window/Drag/FileDrop")
        ;

    this->UIRegisterCallback("UI.Advanced", [this](auto&&...) {
        MulNX::UI::Checkbox("Demo系统", this->showWindow);
        });

    this->SendUIRoot(this->GetName(), [this](auto uico, auto&&...) {
        return this->Window(uico);
        });

    this->SendTask("Update", "DemoSys", [this]() {
        this->Update();
        return true;
        });

    this->PublishAsync("Demo/Refresh"_hash);

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
            std::filesystem::copy(file, this->CS2Paths->demo / file.filename(), std::filesystem::copy_options::overwrite_existing);
        }
        catch (const std::filesystem::filesystem_error& e) {
            this->LogError(I18n("demo.copy_failed", e.what()).c_str());
        }
        this->PublishAsync("Demo/Refresh"_hash);
        break;
    }
    case "Demo/Refresh"_hash: {
        std::unique_lock lock(this->smutex);
        this->demoFiles.clear();
        for (const auto& entry : std::filesystem::directory_iterator(this->CS2Paths->demo)) {
            if (entry.is_regular_file() && entry.path().extension() == ".dem") {
                this->demoFiles.insert(entry.path());
            }
        }
        break;
    }
    case "Demo/Play"_hash: {
        auto& path = msg.asp.get<MulNX::NetExt>()->str1;
        this->AsyncCommand(std::format("playdemo \"{}\"", path));
        break;
    }
    }

}
