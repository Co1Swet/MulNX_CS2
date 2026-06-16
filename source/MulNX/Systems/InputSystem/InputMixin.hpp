#pragma once
#include <MulNX/Core/Module/IModule.hpp>

namespace MulNX {
    class InputSystem;
    template<typename Derived>
    class InputMixin {
    public:
        Derived* This() { return static_cast<Derived*>(this); }

        InputSystem* pInputSystem = nullptr;
        
        InputMixin() {
            This()->delayInits->push_back([this]() {
                this->pInputSystem = static_cast<InputSystem*>(This()->FindModule("InputSystem"));
                return true;
                });
        }
    };
}