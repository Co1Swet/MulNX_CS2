#pragma once
#include <MulNX/Core/Module/IModule.hpp>

namespace MulNX {
    class GlobalVars;
    template<typename T>
    class GlobalVarMixin {
        T* This() { return static_cast<T*>(this); }
    protected:
        GlobalVars* pGlobalVars = nullptr;
    public:
        GlobalVarMixin() {
            This()->preInits.push_back([this]() {
                this->pGlobalVars = static_cast<GlobalVars*>(This()->FindModule("GlobalVars"));
                return true;
                });
        }
    };
}