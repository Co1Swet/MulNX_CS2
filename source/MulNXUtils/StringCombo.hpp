#pragma once
#include <MulNX/Base/UI/UI.hpp>

class StringCombo {
    std::vector<std::string> items;
    size_t currentIndex;
public:
    // 注意，0 位置应当是“默认”
    explicit StringCombo(std::vector<std::string>&& items)
        : items(std::move(items))
        , currentIndex(0) {
    }
    // 渲染函数，接受标签名，返回是否发生改变
    bool Render(const char* label) {
        bool changed = false;
        const char* preview = this->items[this->currentIndex].c_str();

        if (ImGui::BeginCombo(label, preview)) {
            for (int i = 0; i < static_cast<int>(this->items.size()); ++i) {
                bool isSelected = (this->currentIndex == i);
                if (ImGui::Selectable(this->items[i].c_str(), isSelected)) {
                    this->currentIndex = i;
                    changed = true;
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        return changed;
    }

    // 获取当前选中的字符串，const 成员函数，返回 const char*
    const char* GetSelected() const {
        if (this->currentIndex == 0)return nullptr;
        return this->items[this->currentIndex].c_str();
    }
};