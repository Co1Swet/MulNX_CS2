#pragma once
#include <Mirror/SceneSystem/SceneSystem.hpp>

class CloudController final :public CSModuleBase {
    MulReflBool(hideCloud, false);
    MulReflBool(hideSun, false);

    void Menu();
    bool Init()override;
public:
    bool NeedHide(const char* matName);
};