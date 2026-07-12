#pragma once
#include <MulNX/Core/Module/Module.hpp>

namespace MulNX {
    class UICoordinator final :public MulNX::Module<UICoordinator> {
        std::unordered_map<std::string, size_t>nameToIndex{};
        std::vector<MulNX::UINode>UINodes{};
        struct Padding {
            float top = 0;
            float bottom = 0;
            float left = 0;
            float right = 0;
        };
        Padding padding;

        bool Init()override;
        void ProcessMsg(MulNX::Message& msg)override;
        void Window();
    public:
        void HandleUpdate();
        void Render();
        void CallUINode(std::string&& name);
    };
}