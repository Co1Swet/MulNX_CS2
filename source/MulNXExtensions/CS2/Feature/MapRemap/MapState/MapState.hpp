#pragma once
#include <Intro/CSModuleBase.hpp>

class MapState final :public CSModuleBase {
    void Menu();
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;
public:
    std::atomic<std::shared_ptr<std::string>> pRawMapName = nullptr;
    std::atomic<std::shared_ptr<std::string>> pTargetAddonID = nullptr;
    std::atomic<std::shared_ptr<std::string>> pTargetMapName = nullptr;
};

template<typename T>
class RemapMixin {
    T* This() { return static_cast<T*>(this); }
protected:
    MapState* pMapState = nullptr;
    RemapMixin() {
        This()->preInits.push_back([this]() {
            this->pMapState = This()->FindModule<MapState>("MapState");
            return true;
            });
    }
};