#include "UICoordinator.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <MulNX/Systems/Systems.hpp>

void MulNX::UICoordinator::Window() {
    auto w = MulNX::UI::RAIIWindow(I18n("ui.settings").c_str());

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 fullSize = viewport->Size;

    ImGui::SliderFloat(I18n("ui.padding.top").c_str(), &this->padding.top, 0.0f, fullSize.y / 2);
    ImGui::SliderFloat(I18n("ui.padding.bottom").c_str(), &this->padding.bottom, 0.0f, fullSize.y / 2);
    ImGui::SliderFloat(I18n("ui.padding.left").c_str(), &this->padding.left, 0.0f, fullSize.x / 2);
    ImGui::SliderFloat(I18n("ui.padding.right").c_str(), &this->padding.right, 0.0f, fullSize.x / 2);

    ImGui::Separator();

    if (ImGui::BeginTable("Nodes", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_WidthFixed, 50.0f);
        ImGui::TableSetupColumn("Name");
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

    ImGui::Text(I18n("ui.style.info").c_str());
    if (ImGui::Button(I18n("ui.style.save").c_str())) {
        this->PublishAsync("UISystem/SaveStyle"_hash);
    }
    ImGui::Separator();
    ImGui::ShowStyleEditor();
}

bool MulNX::UICoordinator::Init() {
    (*this)
        .SubscribeAsync("UISystem/ModulePush")
        .SubscribeAsync("UISystem/UICallback")
        .SubscribeAsync("UINode/Swap");     // 订阅交换消息

    this->SendUIRoot(this->GetName(), [this](auto&&...) {return this->Window();});
    return true;
}

void MulNX::UICoordinator::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "UISystem/ModulePush"_hash: {
        MulNX::UINode* pNode = msg.asp.get<MulNX::UINode>();
        size_t index = UINodes.size();
        UINodes.emplace_back(std::move(*pNode));
        auto& node = UINodes.back();
        nameToIndex[node.name] = index;
        
        this->LogSucc(std::format("接收到UI节点: {} (索引 {})", node.name, index));
        break;
    }
    case "UISystem/UICallback"_hash: {
        MulNX::UINode node = std::move(*msg.asp.get<MulNX::UINode>());
        auto&& [target, str] = msg.Access<uint64_t, const char*>();
        this->UICallbacks[target].push_back(std::move(node));

        this->LogSucc(std::format("UI节点: {} 回调于： {}", node.name, str));
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
        node.Render(this, nullptr);
    }
}

void MulNX::UICoordinator::CallbackCall(uint64_t hash, Message* msg) {
    auto it = this->UICallbacks.find(hash);
    if (it == this->UICallbacks.end())return;
    for (auto& callback : it->second) {
        callback.Render(this, msg);
    }
}