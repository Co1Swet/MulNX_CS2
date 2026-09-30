#pragma once
#include <Mirror/SceneSystem/SceneSystem.hpp>

class CloudController final :public CSModuleBase {
    std::atomic<bool> hideCloud = false;
    std::atomic<bool> hideSun = false;

    void Menu();
    bool Init()override;
public:
    bool NeedHide(const char* matName);
};