#pragma once
#include <Intro/CSModuleBase.hpp>
#include <MulNXExtensions/WebSocketManager/WebSocketMixin.hpp>

class HSI final :public CSModuleBase, public WebSocketMixin<HSI> {
    bool Init()override;
};