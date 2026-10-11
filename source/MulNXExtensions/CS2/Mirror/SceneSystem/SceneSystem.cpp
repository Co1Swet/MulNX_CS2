#include "SceneSystem.hpp"

bool SceneSystem::Init() {

    this->SubscribeSync("Hook/LoadLibraryExW/scenesystem.dll", [this](MulNX::Message& msg) {
        this->pDrawCurrentPrimitives = (DrawCurrentPrimitives_t)this->CS2->scenesystem.GetTextRegion()
            .FindRegion(CS2::Signatures::SceneSystem::Func_DrawCurrentPrimitives).Data();

        this->pDrawSceneData = (DrawSceneData_t)this->CS2->scenesystem.GetTextRegion()
            .FindRegion(CS2::Signatures::SceneSystem::Func_DrawSceneData).Data();

        this->pInitDrawingData = (InitDrawingData_t)this->CS2->scenesystem.GetTextRegion()
            .FindRegion(CS2::Signatures::SceneSystem::Func_InitDrawingData).Data();
        });

    return true;
}