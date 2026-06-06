#include "UIContext.hpp"

#include <MulNX/Base/UI/UI.hpp>

bool MulNX::UIContext::CallUINode(const std::string& Name) {
    auto ItEntry = this->CallMap.find(Name);
    if (ItEntry == this->CallMap.end())return false;
    MulNXHandle& hUINode = ItEntry->second;
    auto ItUINode = this->UINodeMap.find(hUINode);
    if (ItUINode == this->UINodeMap.end())return false;
    MulNX::UINode& UINode = ItUINode->second;
    UINode.Draw();
    return true;
}

void MulNX::UIContext::Draw() {
    if (this->EnableErrorHandle) {
        ImGui::Begin("错误");
        ImGui::Text("请等待响应");
        if (ImGui::Button("关闭")) {
            this->EnableErrorHandle = false;
        }
        ImGui::End();
    }
    this->Next = this->EntryDraw;
    bool CallResult = true;
    while (CallResult) {
        std::string current = this->Next;
        this->Next = std::string{};
        CallResult = this->CallUINode(current);
    }
}
void MulNX::UIContext::AddUINode(MulNXHandle hUINode, MulNX::UINode&& UINode) {
    this->CallMap[UINode.name] = hUINode;
    UINode.MainContext = this;
    this->UINodeMap[hUINode] = std::move(UINode);
}
MulNX::UINode* MulNX::UIContext::GetUINode(const MulNXHandle& hUINode) {
    auto It = this->UINodeMap.find(hUINode);
    if (It != this->UINodeMap.end()) {
        return &(It->second);
    }
    return nullptr;
}