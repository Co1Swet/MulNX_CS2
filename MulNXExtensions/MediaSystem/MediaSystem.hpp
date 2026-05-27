#pragma once

#include <MulNX/MulNX.hpp>
#include <MulNXExtensions/GraphicsManager/GraphicsManager.hpp>
#include <MulNXExtensions/MediaSystem/D3D11AV.hpp>

class MediaSystem final :public MulNX::ModuleBase {
    bool Window(MulNX::UINode* node);
public:
    bool Init()override;
};