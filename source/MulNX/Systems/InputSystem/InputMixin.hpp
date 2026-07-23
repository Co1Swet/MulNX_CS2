#pragma once
#include <MulNX/Core/Module/IModule.hpp>

namespace MulNX {
    class InputSystem;
    template<typename T>
    class InputMixin {
        T* This() { return static_cast<T*>(this); }
    public:
        InputSystem* pInputSystem = nullptr;
        
        InputMixin() {
            This()->preInits.push_back([this]() {
                this->pInputSystem = static_cast<InputSystem*>(This()->FindModule("InputSystem"));
                return true;
                });
        }
    };
}