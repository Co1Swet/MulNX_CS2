#pragma once

#include <MulNXExtensions/CS2/CSModuleBase.hpp>

class POVFixer final :public CSModuleBase {
    bool enable = false;
    void BeforeDraw();
    void OnSetGlow(MulNX::Message& msg);
    void Draw(MulNX::UINode* node);
public:
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;
};