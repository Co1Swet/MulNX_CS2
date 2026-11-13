#pragma once
#include <Intro/Intros.hpp>

class ESPSkeleton final :public CSModule<ESPSkeleton> {
    std::vector<std::vector<int>> chains;
    MulReflBool(enable, false);
    void Draw();
    void DrawSkelgton(CS2::C_CSPlayerPawn* pPawn);
    bool Init()override;
};