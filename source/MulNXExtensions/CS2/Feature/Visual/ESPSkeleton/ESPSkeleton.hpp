#pragma once
#include <Intro/CSModuleBase.hpp>

class ESPSkeleton final :public CSModuleBase {
    std::vector<std::vector<int>> chains;
    void Draw();
    void DrawSkelgton(CS2::C_CSPlayerPawn* pPawn);
    bool Init()override;
};