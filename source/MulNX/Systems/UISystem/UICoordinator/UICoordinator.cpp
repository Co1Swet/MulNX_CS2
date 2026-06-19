#include "UICoordinator.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <MulNX/Systems/Systems.hpp>

void MulNX::UICoordinator::Window(MulNX::UINode* node) {
    auto w = MulNX::UI::RAIIWindow(I18n("ui.settings").c_str());

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 fullSize = viewport->Size;

    ImGui::SliderFloat(I18n("ui.padding.top").c_str(), &this->padding.top, 0.0f, fullSize.y / 2);
    ImGui::SliderFloat(I18n("ui.padding.bottom").c_str(), &this->padding.bottom, 0.0f, fullSize.y / 2);
    ImGui::SliderFloat(I18n("ui.padding.left").c_str(), &this->padding.left, 0.0f, fullSize.x / 2);
    ImGui::SliderFloat(I18n("ui.padding.right").c_str(), &this->padding.right, 0.0f, fullSize.x / 2);

    ImGui::Separator();

    if (ImGui::BeginTable("Nodes", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_WidthFixed, 50.0f);
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Root");
        ImGui::TableSetupColumn("Actions");
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < UINodes.size(); ++i) {
            auto& displayNode = UINodes[i];
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%zu", i);

            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(displayNode.name.c_str());

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s", displayNode.drawAsARoot ? "Yes" : "No");

            ImGui::TableSetColumnIndex(3);
            ImGui::PushID(static_cast<int>(i));

            // 上移按钮
            if (ImGui::ArrowButton("##up", ImGuiDir_Up)) {
                if (i > 0) {
                    auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("UINode/Swap"_hash);
                    rp->str1 = UINodes[i - 1].name;
                    rp->str2 = UINodes[i].name;
                    this->PublishAsync(std::move(msg));
                }
            }

            ImGui::SameLine();

            // 下移按钮
            if (ImGui::ArrowButton("##down", ImGuiDir_Down)) {
                if (i < UINodes.size() - 1) {
                    auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("UINode/Swap"_hash);
                    rp->str1 = UINodes[i].name;
                    rp->str2 = UINodes[i + 1].name;
                    this->PublishAsync(std::move(msg));
                }
            }

            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    this->CallUINode("UISystem");
}

bool MulNX::UICoordinator::Init() {
    (*this)
        .SubscribeAsync("UISystem/ModulePush")
        .SubscribeAsync("UINode/Swap");     // 订阅交换消息
    // 向 UISystem 注册本模块的根窗口回调
    this->SendUIRoot(this->GetName(), [this](MulNX::UINode* node) {
        return this->Window(node);
        });
    return true;
}

void MulNX::UICoordinator::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "UISystem/ModulePush"_hash: {
        MulNX::UINode* pNode = msg.asp.get<MulNX::UINode>();
        if (!pNode) break;

        size_t index = UINodes.size();
        UINodes.emplace_back(std::move(*pNode));
        auto& node = UINodes.back();
        nameToIndex[node.name] = index;
        node.pCoordinator = this;

        this->LogSucc(std::format("接收到UI节点: {} (索引 {})", node.name, index));
        break;
    }

    case "UINode/Swap"_hash: {
        auto pNet = msg.asp.get<MulNX::NetExt>();
        if (!pNet) break;

        const auto& name1 = pNet->str1;
        const auto& name2 = pNet->str2;

        auto it1 = nameToIndex.find(name1);
        auto it2 = nameToIndex.find(name2);
        if (it1 == nameToIndex.end() || it2 == nameToIndex.end())
            break;

        size_t idx1 = it1->second;
        size_t idx2 = it2->second;

        // 执行交换并同步映射
        std::swap(UINodes[idx1], UINodes[idx2]);
        nameToIndex[UINodes[idx1].name] = idx1;
        nameToIndex[UINodes[idx2].name] = idx2;

        this->LogSucc(std::format("交换节点: {} <-> {}", name1, name2));
        break;
    }
    }
}

void MulNX::UICoordinator::HandleUpdate() {
    this->Update();   // 调用基类 Module 的 Update
}

void MulNX::UICoordinator::Render() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 fullSize = viewport->Size;

    ImVec2 dockPos(
        viewport->Pos.x + this->padding.left,
        viewport->Pos.y + this->padding.top
    );
    ImVec2 dockSize(
        fullSize.x - this->padding.left - this->padding.right,
        fullSize.y - this->padding.top - this->padding.bottom
    );

    ImGui::SetNextWindowPos(dockPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(dockSize, ImGuiCond_Always);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_DockingEmptyBg, IM_COL32(0, 0, 0, 0));

    ImGui::Begin("DockRoot", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoBackground);
    ImGui::DockSpace(ImGui::GetID("MainDockSpace"), ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);
    ImGui::End();

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);

    for (auto& node : UINodes) {
        if (node.drawAsARoot) {
            node.Draw();
        }
    }
}

void MulNX::UICoordinator::CallUINode(std::string&& name) {
    auto it = nameToIndex.find(name);
    if (it != nameToIndex.end()) {
        UINodes[it->second].Draw();
    }
}