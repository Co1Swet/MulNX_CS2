#include "RetEditor.hpp"
#include <MulNX/Base/UI/UI.hpp>

void RetEditor::Render() {
    std::unique_lock lock(this->mutex);

    ImGui::SeparatorText("检测到的调用点");
    if (this->detected.empty()) {
        ImGui::TextDisabled("（空）");
        return;
    }

    for (const auto& call : this->detected) {
        ImGui::Text("%llX", call);                      // 十六进制显示
        ImGui::SameLine();
        if (this->setted.find(call) != this->setted.end()) {
            ImGui::TextDisabled("已添加");
        }
        else {
            // 使用 call 作为 ID 后缀，保证按钮唯一
            if (ImGui::Button(("添加##" + std::to_string(call)).c_str())) {
                this->setted.insert(call);
            }
        }
    }

    ImGui::SeparatorText("已强制返回的调用点");
    if (this->setted.empty()) {
        ImGui::TextDisabled("（空）");
        return;
    }
    std::vector<uintptr_t> toRemove;
    for (auto it = this->setted.begin();it != this->setted.end();) {
        ImGui::Text("%llX", *it);
        ImGui::SameLine();
        if (ImGui::Button(("移除##" + std::to_string(*it)).c_str())) {
            it = this->setted.erase(it);
        }
        else ++it;

    }
}
bool RetEditor::Check(MulNX::Hook* hk, RegContext* ctx) {
    auto returnAddress = *(uintptr_t*)hk->GetRawStackAddr(ctx);
    std::unique_lock lock(this->mutex);

    this->detected.insert(returnAddress);
    if (this->setted.find(returnAddress) == this->setted.end())return false;

    return true;
}