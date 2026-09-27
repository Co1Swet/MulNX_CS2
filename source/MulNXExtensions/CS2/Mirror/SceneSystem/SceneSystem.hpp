#pragma once
#include <Intro/CSModuleBase.hpp>

class SceneSystem final :public CSModuleBase {
    MulNX::Memory::DllModule scenesystem{};
    class MaterialSystem* pMaterialSystem = nullptr;
    void* pRaw = nullptr;

    std::unique_ptr<MulNX::Hook> hkDrawSceneData = nullptr;

    bool Init()override;
};