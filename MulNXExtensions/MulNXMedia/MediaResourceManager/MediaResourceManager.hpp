#pragma once

#include <MulNX/MulNX.hpp>
#include <MulNXExtensions/GraphicsManager/GraphicsManager.hpp>
#include <MulNXExtensions/MulNXMedia/D3D11AV.hpp>

class MediaResourceManager final :public MulNX::ModuleBase {    
    bool Window(MulNX::UINode* node);
public:
    bool Init()override;
};