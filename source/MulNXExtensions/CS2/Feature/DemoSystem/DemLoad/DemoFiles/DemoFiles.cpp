#include "DemoFiles.hpp"

void DemoFiles::Menu() {
    std::unique_lock lock(this->smutex);

    if (ImGui::Button(I18n("demo.refresh").c_str())) {
        this->PublishAsync("Demo/Refresh"_hash);
    }

    if (this->demoFiles.empty()) {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", I18n("demo.empty").c_str());
    }

    // 可复制的列表控件标识
    int chooseSize = 0;
    int fisrt = -1;
    {
        float childHeight = ImGui::GetContentRegionAvail().y * 0.65f;
        auto c = MulNX::UI::RAIIChild("DemoFileList", ImVec2(0, childHeight), true);

        // 遍历文件集合（set 自动按路径排序）
        for (int i = 0;i < this->demoFiles.size();++i) {
            auto& demFile = this->demoFiles[i];
            // 用 Selectable 展示条目，支持高亮
            std::string fileName = std::format("{}    ---{}",
                demFile.path.filename().string(),
                demFile.anylized ? "已分析" : "未分析");

            if (ImGui::Selectable(fileName.c_str(), demFile.beChoosing, ImGuiSelectableFlags_AllowDoubleClick)) {
                demFile.beChoosing = !demFile.beChoosing;
            }
            if (demFile.beChoosing) {
                ++chooseSize;
                if (fisrt == -1)fisrt = i;
            }
        }
    }

    if (chooseSize == 0) {
        ImGui::Text("当前未选中任何Demo");
        return;
    }
    if (chooseSize > 1) {
        if (ImGui::Button("分析所有")) {
            for (auto& demFile : this->demoFiles) {
                if(!demFile.beChoosing)continue;
                auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/Analyze"_hash);
                rp->str1 = demFile.path.string();
                this->PublishAsync(std::move(msg));
            }
        }
        return;
    }

    const auto& demFile = this->demoFiles[fisrt];
    // 底部操作按钮
    if (ImGui::Button("播放所选")) {
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/Play"_hash);
        rp->str1 = demFile.path.string();
        this->PublishAsync(std::move(msg));
    }
    if (ImGui::Button("分析所选")) {
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/Analyze"_hash);
        rp->str1 = demFile.path.string();
        this->PublishAsync(std::move(msg));
    }
    if (ImGui::Button("尝试加载所选分析文件")) {
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/JSON/Load"_hash);
        rp->str1 = demFile.path.stem().string();
        this->PublishAsync(std::move(msg));
    }
    if (ImGui::Button("复制路径到剪贴板")) {
        ImGui::SetClipboardText(demFile.path.string().c_str());
    }
}

bool DemoFiles::Init() {
    this->dirData = this->Path()->PathGetForShared("Data");

    (*this)
        .SubscribeAsync("Demo/Refresh")
        ;

    this->UIRegisterCallback("UI.Demo.Main", [this](auto&&...) {
        this->Menu();
        });

    this->SendTask("Update", "DemoSys", [this]() {
        this->Update();
        return true;
        });

    return true;
}

void DemoFiles::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Demo/Refresh"_hash: {
        std::unique_lock lock(this->smutex);
        this->demoFiles.clear();
        for (const auto& entry : std::filesystem::directory_iterator(this->CS2Paths->demo)) {
            if (!entry.is_regular_file() || entry.path().extension() != ".dem")continue;
            DemoFile demFile{};
            demFile.path = entry.path();
            if (std::filesystem::exists(this->dirData / (entry.path().stem().string() + ".json"))) {
                demFile.anylized = true;
            }
            this->demoFiles.push_back(std::move(demFile));
        }
        break;
    }

    }
}