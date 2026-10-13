#pragma once
#include "DemState/DemState.hpp"

template<typename T>
class DemModuleMixin {
    T* This() { return static_cast<T*>(this); }
protected:
    DemState* pDemState = nullptr;
    DemModuleMixin() {
        This()->preInits.push_back([this]() {
            This()->pDemState = This()->FindModule<DemState>("DemState");
            return true;
            });
    }
};

class DemModuleBase :public CSModuleBase, public DemModuleMixin<DemModuleBase> {};