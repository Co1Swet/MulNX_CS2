#include "SceneSystem.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <Mirror/MaterialSystem/MaterialSystem.hpp>

bool SceneSystem::Init() {
    this->pMaterialSystem = this->FindModule<MaterialSystem>("MaterialSystem");

    this->SubscribeSync("Hook/LoadLibraryExW/scenesystem.dll", [this](MulNX::Message& msg) {return this->OnSceneSystemLoad(msg);});


    return true;
}