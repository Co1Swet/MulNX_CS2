#pragma once
#include <MulNX/MulNX.hpp>

class TimeLiner :public MulNX::Module<TimeLiner> {
    float m_clickRatio = 0.0f;
    void Menu(MulNX::UINode* node);
    bool Init()override;
public:
    class ITimeAdapter* pTimeAdapter = nullptr;
};