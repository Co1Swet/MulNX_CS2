#pragma once
#include <MulNX/Base/UI/UI.hpp>
#include <atomic>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class StringCombo {
    const std::vector<std::string> items;
    std::shared_ptr<std::string> cachedValue;
    std::optional<size_t> cachedIndex;
    bool cacheInitialized = false;

public:
    explicit StringCombo(std::vector<std::string>&& items)
        : items(std::move(items)) {}

    bool Render(const char* label,std::atomic<std::shared_ptr<std::string>>& value) {
        auto selected = value.load(std::memory_order_acquire);
        if (!this->cacheInitialized || selected != this->cachedValue) {
            this->cachedValue = selected;
            this->cachedIndex.reset();
            if (selected) {
                for (size_t i = 0; i < this->items.size(); ++i) {
                    if (this->items[i] == *selected) {
                        this->cachedIndex = i;
                        break;
                    }
                }
            }
            this->cacheInitialized = true;
        }

        const bool isEmpty = !selected || selected->empty();
        const char* preview = isEmpty ? "默认" : selected->c_str();
        
        if (!ImGui::BeginCombo(label, preview))return false;
        auto close = scope_exit([&]() {
            ImGui::EndCombo();
            });

        bool changed = false;
        if (ImGui::Selectable("默认", isEmpty) && !isEmpty) {
            value.store(std::make_shared<std::string>(), std::memory_order_release);
            this->cachedValue.reset();
            this->cachedIndex.reset();
            changed = true;
        }
        if (isEmpty) {
            ImGui::SetItemDefaultFocus();
        }
        for (size_t i = 0; i < this->items.size(); ++i) {
            bool isSelected = this->cachedIndex == i;
            if (ImGui::Selectable(this->items[i].c_str(), isSelected)) {
                auto newValue = std::make_shared<std::string>(this->items[i]);
                value.store(newValue, std::memory_order_release);
                this->cachedValue = std::move(newValue);
                this->cachedIndex = i;
                changed = true;
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }

        return changed;
    }
};