#pragma once

#include <MulNX/Systems/MessageManager/MessageManager.hpp>
#include <functional>

namespace MulNX {
    class UIContext;
    class UINode {
    public:
        std::string name{};
        std::function<void(UINode*)>MyFunc = nullptr;
        MulNX::MessageManager* pMsgManager = nullptr;

        // 按照线程管理进行成员分类

        // 初始化即可
        MulNXHandle hSelf{};
        MulNXHandle HModule{};

        // 跨线程数据

        bool Active = true;

        MulNX::UIContext* MainContext = nullptr;

        MulNX::UINode() = default;
        MulNX::UINode(const MulNX::UINode&) = default;
        MulNX::UINode(MulNX::UINode&&) = default;
        MulNX::UINode& operator=(const MulNX::UINode&) = default;
        MulNX::UINode& operator=(MulNX::UINode&&) = default;

        void Draw();

        bool CallUINode(std::string&& Name);
        bool SetNextUINode(std::string&& Name);

        static MulNX::UINode Create(MulNX::ModuleBase* MB);
    };
}