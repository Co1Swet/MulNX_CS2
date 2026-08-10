#pragma once
#include "BackgroundEntityScan.hpp"

class IEntityIterationModule {
public:
    virtual void OnItBegin() {};
    virtual void OnItEntity(int index, CS2::C_BaseEntity*) {};
    virtual void OnItPlayer(int index, CS2::CCSPlayerController*, CS2::C_CSPlayerPawn*) {};
    virtual void OnItEnd() {};
};

template<typename T>
class EntityIterationMixin :public IEntityIterationModule {
    T* This() { return static_cast<T*>(this); }
protected:
    BackgroundEntityScan* pBackgroundEntityScan = nullptr;
public:
    EntityIterationMixin() {
        This()->postInits.push_back([this]()->bool {
            this->pBackgroundEntityScan = This()->FindModule<BackgroundEntityScan>("BackgroundEntityScan");
            this->pBackgroundEntityScan->ParticipateItCSModules.push_back(this);
            return true;
            });
    }
};