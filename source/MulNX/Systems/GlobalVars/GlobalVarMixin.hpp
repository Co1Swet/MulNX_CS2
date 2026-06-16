#pragma once
#include <MulNX/Core/Module/IModule.hpp>

namespace MulNX {
    class GlobalVars;
    template<typename Derived>
    class GlobalVarMixin {
    protected:
        GlobalVars* pGlobalVars = nullptr;
    public:
        Derived* This() { return static_cast<Derived*>(this); }
        GlobalVarMixin() {
            This()->delayInits->push_back([this]() {
                this->pGlobalVars = static_cast<GlobalVars*>(This()->FindModule("GlobalVars"));
                return true;
                });
        }
    };
}