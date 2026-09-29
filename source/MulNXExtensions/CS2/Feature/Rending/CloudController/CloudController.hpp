#pragma once
#include <Mirror/SceneSystem/SceneSystem.hpp>
#include <d3d11.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class CloudController final :public CSModuleBase {
    SceneSystem* pSceneSystem = nullptr;

    std::atomic<bool> hideCloud = false;
    std::atomic<bool> hideSun = false;

    ComPtr<ID3D11BlendState> pNoDrawBlend = nullptr;
    ComPtr<ID3D11DepthStencilState> pNoDrawDepth = nullptr;

    bool bBlockColor = false;
    bool bBlockDepth = false;

    std::unique_ptr<MulNX::Hook> hkDrawSceneData = nullptr;

    std::unique_ptr<MulNX::Hook> hkOMSetBlendState = nullptr;
    std::unique_ptr<MulNX::Hook> hkOMSetDepthStencilState = nullptr;

    SceneSystem::DrawCurrentPrimitives_t pDrawCurrentPrimitives = nullptr;
    SceneSystem::DrawSceneData_t pDrawSceneData = nullptr;

    void Menu();
    bool Init()override;
    bool NeedHide(const char* matName);
};