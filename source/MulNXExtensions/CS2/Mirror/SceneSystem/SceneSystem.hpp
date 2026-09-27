#pragma once
#include <Intro/CSModuleBase.hpp>

class SceneSystem final :public CSModuleBase {
    class MaterialSystem* pMaterialSystem = nullptr;
    void* pRaw = nullptr;

    bool Init()override;
};