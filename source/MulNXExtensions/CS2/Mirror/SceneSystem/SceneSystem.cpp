#include "SceneSystem.hpp"

bool SceneSystem::Init() {

    this->SubscribeSync("Hook/LoadLibraryExW/scenesystem.dll", [this](MulNX::Message& msg) {
        this->scenesystem = MulNX::Memory::DllModule(L"scenesystem.dll");

        this->pDrawCurrentPrimitives = (DrawCurrentPrimitives_t)this->scenesystem.GetTextRegion()
            .FindRegion(CS2::Signatures::SceneSystem::Func_DrawCurrentPrimitives).Data();

        this->pDrawSceneData = (DrawSceneData_t)this->scenesystem.GetTextRegion()
            .FindRegion(CS2::Signatures::SceneSystem::Func_DrawSceneData).Data();
        });

    return true;
}