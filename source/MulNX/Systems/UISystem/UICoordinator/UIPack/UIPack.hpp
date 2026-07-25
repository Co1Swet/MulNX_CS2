#pragma once
#include <MulNX/Systems/UISystem/UINode/UINode.hpp>

namespace MulNX {
    class UIPack {
        std::unordered_map<std::string, size_t>nameToIndex{};
    public:
        std::vector<MulNX::UINode>nodes{};
        std::pair<MulNX::UINode&, size_t> Push(MulNX::UINode&& node);
        std::optional<std::pair<std::string, std::string>>Render(const char* id);
        bool Swap(const std::string& name1, const std::string& name2);
    };
}