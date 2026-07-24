#pragma once
#include <MulNX/Core/Module/ModuleBase.hpp>

namespace MulNX {
    class UICoordinator;
    class UINode {
    public:
        std::string name{};
        std::function<void(UICoordinator*, MulNX::Message* msg)>Render = nullptr;
        MulNXHandle hSelf{};
        MulNXHandle HModule{};
        static MulNX::UINode Create(MulNX::ModuleBase* MB);
    };
}