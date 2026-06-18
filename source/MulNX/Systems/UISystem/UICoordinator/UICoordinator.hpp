#pragma once
#include <MulNX/Core/Module/Module.hpp>

namespace MulNX {
    class UICoordinator final :public MulNX::Module<UICoordinator> {
    public:
        bool Init()override;
    };
}