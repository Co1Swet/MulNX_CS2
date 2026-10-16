#pragma once
#include <Feature/DemoSystem/DemoSystem.hpp>

template<typename T>
class DemModuleMixin {
    T* This() { return static_cast<T*>(this); }
protected:
    DemoSystem* pDemSys = nullptr;
    DemModuleMixin() {
        This()->preInits.push_back([this]() {
            This()->pDemSys = This()->FindModule<DemoSystem>("DemoSystem");
            return true;
            });
    }
};

class DemModuleBase :public CSModuleBase, public DemModuleMixin<DemModuleBase> {};