#pragma once
#include <Intro/CSModuleBase.hpp>

class ESPSkeleton final :public CSModuleBase {
    std::vector<std::vector<int>> chains;
    void Draw(MulNX::UINode* node);
    void DrawSkelgton(CS2::C_CSPlayerPawn* pPawn);
    bool Init()override;
};