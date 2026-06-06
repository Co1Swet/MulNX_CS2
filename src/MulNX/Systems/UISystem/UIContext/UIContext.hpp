#pragma once

#include "UINode/UINode.hpp"

#include <queue>

namespace MulNX {
    class UIContext {
    private:
        friend class MulNX::UINode;
        bool CallUINode(const std::string& Name);
    public:
        bool Active = true;
        MulNX::Core::Core* Core = nullptr;
        bool EnableErrorHandle = false;

        // 入口点字符串
        std::string EntryDraw{};
        // 由字符串映射到句柄
        std::unordered_map<std::string, MulNXHandle>CallMap{};
        // 然后从句柄得到具体的UI节点
        std::unordered_map<MulNXHandle, MulNX::UINode>UINodeMap{};
        // 下一个要调用的UI节点名称
        std::string Next;

        void Draw();
        void AddUINode(MulNXHandle hUINode, MulNX::UINode&& UINode);
        MulNX::UINode* GetUINode(const MulNXHandle& hUINode);
    };
}