#pragma once

#include <MulNX/MulNX.hpp>

class MiniMap final :public MulNX::ModuleBase {
private:
    // 可配置项：地图像素大小（正方形），世界->像素 缩放，中心玩家索引，是否自动适配
    float MapSize = 200.0f; // 默认 200x200
    float Zoom = 0.15f;     // world units -> pixels
    float Radius = 12.0f; // 玩家图标半径

    // 上次点击的玩家索引（0 表示无）
    int LastClickedPlayer = 0;
    // 是否随窗口大小联动
    bool FollowWindow = true;
public:
    bool Init()override;
    void Main();
    bool UINodeFunc(MulNX::UINode* node);
};