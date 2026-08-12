#include "UICoordinator.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <MulNX/Systems/Systems.hpp>

void MulNX::UICoordinator::Window() {
    auto w = MulNX::UI::RAIIWindow(I18n("ui.settings").c_str());
    if (!w)return;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 fullSize = viewport->Size;

    ImGui::SliderFloat(I18n("ui.padding.top").c_str(), &this->padding.top, 0.0f, fullSize.y / 2);
    ImGui::SliderFloat(I18n("ui.padding.bottom").c_str(), &this->padding.bottom, 0.0f, fullSize.y / 2);
    ImGui::SliderFloat(I18n("ui.padding.left").c_str(), &this->padding.left, 0.0f, fullSize.x / 2);
    ImGui::SliderFloat(I18n("ui.padding.right").c_str(), &this->padding.right, 0.0f, fullSize.x / 2);

    ImGui::SeparatorText("背景UI节点");
    if (auto action = this->backgoundPack.Render("bk")) {
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("UINode/Swap/Background"_hash);
        rp->str1 = action->first;
        rp->str2 = action->second;
        this->PublishAsync(std::move(msg));
    }
    ImGui::SeparatorText("窗口化UI节点");
    if (auto action = this->midPack.Render("mid")) {
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("UINode/Swap"_hash);
        rp->str1 = action->first;
        rp->str2 = action->second;
        this->PublishAsync(std::move(msg));
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
        .SubscribeAsync("UISystem/ModulePush/Background")
        .SubscribeAsync("UINode/Swap")
        .SubscribeAsync("UINode/Swap/Background")
        .SubscribeAsync("UISystem/UICallback")
        ;

    this->SendUIRoot(this->GetName(), [this](auto&&...) {return this->Window();});
    return true;
}

void MulNX::UICoordinator::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "UISystem/UICallback"_hash: {
        MulNX::UINode node = std::move(*msg.asp.get<MulNX::UINode>());
        auto&& [target, str] = msg.Access<uint64_t, const char*>();
        this->UICallbacks[target].push_back(std::move(node));

        this->LogSucc(std::format("UI节点: {} 回调于： {}", node.name, str));
        break;
    }
    case "UISystem/ModulePush"_hash: {
        MulNX::UINode* pNode = msg.asp.get<MulNX::UINode>();
        auto&& [node, idx] = this->midPack.Push(std::move(*pNode));
        this->LogSucc(std::format("接收到UI节点: {} (索引 {})", node.name, idx));
        break;
    }
    case "UINode/Swap"_hash: {
        auto pNet = msg.asp.get<MulNX::NetExt>();
        if (!pNet) break;
        if (this->midPack.Swap(pNet->str1, pNet->str2)) {
            this->LogSucc(std::format("交换节点: {} <-> {}", pNet->str1, pNet->str2));
        }
        else {
            this->LogError(std::format("交换节点失败: {} <-> {}", pNet->str1, pNet->str2));
        }
        break;
    }
    case "UISystem/ModulePush/Background"_hash: {
        MulNX::UINode* pNode = msg.asp.get<MulNX::UINode>();
        auto&& [node, idx] = this->backgoundPack.Push(std::move(*pNode));
        this->LogSucc(std::format("接收到UI节点: {} (索引 {})", node.name, idx));
        break;
    }
    case "UINode/Swap/Background"_hash: {
        auto pNet = msg.asp.get<MulNX::NetExt>();
        if (!pNet) break;
        if (this->backgoundPack.Swap(pNet->str1, pNet->str2)) {
            this->LogSucc(std::format("交换节点: {} <-> {}", pNet->str1, pNet->str2));
        }
        else {
            this->LogError(std::format("交换节点失败: {} <-> {}", pNet->str1, pNet->str2));
        }
        break;
    }
    }
}

void MulNX::UICoordinator::HandleUpdate() {
    this->Update();   // 调用基类 Module 的 Update
}

void MulNX::UICoordinator::Render(bool mid) {
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

    for (auto& node : this->backgoundPack.nodes) {
        node.Render(this, nullptr);
    }

    if (!mid)return;

    for (auto& node : this->midPack.nodes) {
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