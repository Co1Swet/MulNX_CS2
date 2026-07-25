#include "UIPack.hpp"
#include <MulNX/Base/UI/UI.hpp>

std::optional<std::pair<std::string, std::string>> MulNX::UIPack::Render(const char* id) {
    auto t = MulNX::UI::RAIITable(id, { "Index" ,"Name" ,"Actions" }, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
    if (!t)return std::nullopt;

    for (size_t i = 0; i < this->nodes.size(); ++i) {
        auto& displayNode = this->nodes[i];
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
                ImGui::PopID();
                return std::pair{ this->nodes[i - 1].name, this->nodes[i].name };
            }
        }

        ImGui::SameLine();

        // 下移按钮
        if (ImGui::ArrowButton("##down", ImGuiDir_Down)) {
            if (i < this->nodes.size() - 1) {
                ImGui::PopID();
                return std::pair{ this->nodes[i].name, this->nodes[i + 1].name };
            }
        }

        ImGui::PopID();
    }
    return std::nullopt;
}
std::pair<MulNX::UINode&, size_t> MulNX::UIPack::Push(MulNX::UINode&& node) {
    size_t index = this->nodes.size();
    this->nodes.emplace_back(node);
    auto& back = this->nodes.back();
    this->nameToIndex[back.name] = index;
    return { back,index };
}

bool MulNX::UIPack::Swap(const std::string& name1, const std::string& name2) {
    auto it1 = this->nameToIndex.find(name1);
    auto it2 = this->nameToIndex.find(name2);
    if (it1 == this->nameToIndex.end() || it2 == this->nameToIndex.end())
        return false;

    size_t idx1 = it1->second;
    size_t idx2 = it2->second;

    // 执行交换并同步映射
    std::swap(this->nodes[idx1], this->nodes[idx2]);
    this->nameToIndex[this->nodes[idx1].name] = idx1;
    this->nameToIndex[this->nodes[idx2].name] = idx2;

    return true;
}