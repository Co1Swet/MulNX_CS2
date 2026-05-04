#include "MiniMap.hpp"

#include <MulNX/MulNX.hpp>
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/CS2/MulNXCS2Ext.hpp>

#include <algorithm>
#include <string>
#include <sstream>
#include <cfloat>
#include <Windows.h>

bool MiniMap::Init() {
    this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {return this->UINodeFunc(node);});
    this->SendTask("MulNXMain", [this]()->bool {
        this->Main();
        return true;
        });
    return true;
}

void MiniMap::Main() {
    if (this->pInputSystem->CheckWithPack(MulNX::KeyCheckPack{ true,false,false,true,'M',1 })) {
        this->ShowWindow = !this->ShowWindow;
    }

    return;
}

bool MiniMap::UINodeFunc(MulNX::UINode* ThisNode) {
    if (!this->ShowWindow) return true;

    // 保存当前样式
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4 oldWindowBg = style.Colors[ImGuiCol_WindowBg];
    ImVec4 oldChildBg = style.Colors[ImGuiCol_ChildBg];
    ImVec4 oldBorder = style.Colors[ImGuiCol_Border];

    // 设置透明样式
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f); // 完全透明窗口背景
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);  // 完全透明子窗口背景
    style.Colors[ImGuiCol_Border] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);   // 透明边框

    static bool op;
    op = this->ShowWindow;
    ImGui::Begin("小地图窗口", &op);
    this->ShowWindow = op;

    if (false) {
        // 恢复样式
        style.Colors[ImGuiCol_WindowBg] = oldWindowBg;
        style.Colors[ImGuiCol_ChildBg] = oldChildBg;
        style.Colors[ImGuiCol_Border] = oldBorder;
        ImGui::End();
        return true;
    }
    // 计算地图绘制区域：如果随窗口联动则使用剩余内容区域的正方形
    ImVec2 mapSizeVec;
    if (this->FollowWindow) {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        float size = (avail.x < avail.y) ? avail.x : avail.y;

        mapSizeVec = ImVec2(size, size);
    }
    else {
        mapSizeVec = ImVec2(this->MapSize, this->MapSize);
    }

    ImGui::BeginChild("##小地图", mapSizeVec, true, ImGuiWindowFlags_NoMove);

    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    ImVec2 TopLeft = ImGui::GetCursorScreenPos();
    ImVec2 BottomRight = ImVec2(TopLeft.x + mapSizeVec.x, TopLeft.y + mapSizeVec.y);
    ImU32 bgCol = IM_COL32(0, 0, 0, 0);
    DrawList->AddRectFilled(TopLeft, BottomRight, bgCol);

    // 计算缩放（世界单位 -> 像素）
    float worldToPixel = this->Zoom;


    // 屏幕中心点
    ImVec2 centerScreen = ImVec2(TopLeft.x + mapSizeVec.x * 0.5f, TopLeft.y + mapSizeVec.y * 0.5f);


    // 获取鼠标相对于地图左上角的坐标，用于点击检测
    ImVec2 mousePos = ImGui::GetIO().MousePos;
    bool mouseInMap = (mousePos.x >= TopLeft.x && mousePos.x <= BottomRight.x && mousePos.y >= TopLeft.y && mousePos.y <= BottomRight.y);

    // 绘制玩家并处理点击
    for (int i = 1; i <= 10; ++i) {
        std::shared_lock lk(this->AL3D->smutex);
        D_Player& Player = this->AL3D->GetPlayerMsg(i);
        if (!Player.Alive)continue;
        ImVec2 PositionInMiniMap = ImVec2(centerScreen.x + Player.Position.x * worldToPixel, centerScreen.y - Player.Position.y * worldToPixel);

        ImU32 col;
        if (Player.Team == 3) {
            col = IM_COL32(80, 150, 255, 255); // CS2 CT蓝色
        }
        else {
            col = IM_COL32(255, 200, 50, 255); // CS2 T黄色
        }

        DrawList->AddCircleFilled(PositionInMiniMap, this->Radius, col);
        // 编号
        std::string strIndex;
        if (i == 10) {
            strIndex = "0";
        }
        else {
            strIndex = std::to_string(i);
        }
        ImVec2 text_size = ImGui::CalcTextSize(strIndex.c_str());
        DrawList->AddText(ImVec2(PositionInMiniMap.x - text_size.x / 2, PositionInMiniMap.y - text_size.y / 2),
            IM_COL32(255, 255, 255, 220), strIndex.c_str());

        // 点击检测：鼠标在地图内且按下鼠标左键，并且点击位置在图标半径内
        if (mouseInMap && ImGui::IsMouseClicked(0)) {
            float dist2 = (mousePos.x - PositionInMiniMap.x) * (mousePos.x - PositionInMiniMap.x) + (mousePos.y - PositionInMiniMap.y) * (mousePos.y - PositionInMiniMap.y);
            if (dist2 <= (this->Radius + 4.0f) * (this->Radius + 4.0f)) {
                this->LastClickedPlayer = i;
                std::ostringstream oss;
                oss << "点击玩家: " << i << " 位置: (" << Player.Position.x << ", " << Player.Position.y << ", " << Player.Position.z << ")";
                this->ISys().LogInfo(oss.str().c_str());
                MulNX::Message ClickMsg("Command/SpecPlayer"_hash);
                ClickMsg.p1.low<int>() = i;
                this->ISys().PublishAsync(std::move(ClickMsg));
            }
        }
    }

    // 显示上次点击信息
    if (this->LastClickedPlayer != 0) {
        ImGui::Text("上次点击玩家: %d", this->LastClickedPlayer);
    }

    // 绘制中心点十字
    DrawList->AddLine(ImVec2(centerScreen.x - 6, centerScreen.y), ImVec2(centerScreen.x + 6, centerScreen.y), IM_COL32(200, 200, 200, 200));
    DrawList->AddLine(ImVec2(centerScreen.x, centerScreen.y - 6), ImVec2(centerScreen.x, centerScreen.y + 6), IM_COL32(200, 200, 200, 200));

    ImGui::EndChild();

    ImGui::End();
    // 恢复原始样式
    style.Colors[ImGuiCol_WindowBg] = oldWindowBg;
    style.Colors[ImGuiCol_ChildBg] = oldChildBg;
    style.Colors[ImGuiCol_Border] = oldBorder;
    return true;
}