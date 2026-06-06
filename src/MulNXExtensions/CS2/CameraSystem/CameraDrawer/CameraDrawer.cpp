#include "CameraDrawer.hpp"

void CameraModel::Init(float CameraHigh, float CameraX, float CameraY, float AxisLenth, ImU32 Colour) {
    this->Origin = { 0,0,0 };

    this->Lens[0] = { CameraHigh,-CameraX / 2,CameraY / 2 };
    this->Lens[1] = { CameraHigh,-CameraX / 2,-CameraY / 2 };
    this->Lens[2] = { CameraHigh,CameraX / 2,-CameraY / 2 };
    this->Lens[3] = { CameraHigh,CameraX / 2,CameraY / 2 };

    this->X = { AxisLenth,0,0 };
    this->Y = { 0,AxisLenth,0 };
    this->Z = { 0,0,AxisLenth };

    this->Colour = Colour;
}

void CameraModel::Rotate(const DirectX::XMFLOAT3& Rotation) {
    DirectX::XMVECTOR quat = MulNX::Math::CSEulerToQuatVec(Rotation);
    quat = DirectX::XMQuaternionNormalize(quat);

    auto rotatePoint = [&](const DirectX::XMFLOAT3& point) {
        DirectX::XMFLOAT3 result;
        DirectX::XMStoreFloat3(&result, DirectX::XMVector3Rotate(DirectX::XMLoadFloat3(&point), quat));
        return result;
    };

    this->Origin = rotatePoint(this->Origin);
    this->Lens[0] = rotatePoint(this->Lens[0]);
    this->Lens[1] = rotatePoint(this->Lens[1]);
    this->Lens[2] = rotatePoint(this->Lens[2]);
    this->Lens[3] = rotatePoint(this->Lens[3]);
    this->X = rotatePoint(this->X);
    this->Y = rotatePoint(this->Y);
    this->Z = rotatePoint(this->Z);
}

void CameraModel::Move(const DirectX::XMFLOAT3& TargetPoint) {
    MulNX::Math::MovePoint(this->Origin, TargetPoint);

    MulNX::Math::MovePoint(this->Lens[0], TargetPoint);
    MulNX::Math::MovePoint(this->Lens[1], TargetPoint);
    MulNX::Math::MovePoint(this->Lens[2], TargetPoint);
    MulNX::Math::MovePoint(this->Lens[3], TargetPoint);

    MulNX::Math::MovePoint(this->X, TargetPoint);
    MulNX::Math::MovePoint(this->Y, TargetPoint);
    MulNX::Math::MovePoint(this->Z, TargetPoint);
}

void CameraModel::ToScreen(const float* const pMatrix, const float WinWidth, const float WinHeight) {
    MulNX::Math::WorldToScreen(this->Origin, this->Origin2D, pMatrix, WinWidth, WinHeight);

    MulNX::Math::WorldToScreen(this->Lens[0], this->Lens2D[0], pMatrix, WinWidth, WinHeight);
    MulNX::Math::WorldToScreen(this->Lens[1], this->Lens2D[1], pMatrix, WinWidth, WinHeight);
    MulNX::Math::WorldToScreen(this->Lens[2], this->Lens2D[2], pMatrix, WinWidth, WinHeight);
    MulNX::Math::WorldToScreen(this->Lens[3], this->Lens2D[3], pMatrix, WinWidth, WinHeight);

    MulNX::Math::WorldToScreen(this->X, this->X2D, pMatrix, WinWidth, WinHeight);
    MulNX::Math::WorldToScreen(this->Y, this->Y2D, pMatrix, WinWidth, WinHeight);
    MulNX::Math::WorldToScreen(this->Z, this->Z2D, pMatrix, WinWidth, WinHeight);
}

void CameraModel::Draw(const char* Label) {
    auto* drawList = ImGui::GetBackgroundDrawList();
    // 绘制坐标系
    drawList->AddLine(ImVec2(this->Origin2D.x, this->Origin2D.y), ImVec2(this->X2D.x, this->X2D.y), IM_COL32(255, 0, 0, 255), 4.0f);
    drawList->AddLine(ImVec2(this->Origin2D.x, this->Origin2D.y), ImVec2(this->Y2D.x, this->Y2D.y), IM_COL32(0, 255, 0, 255), 4.0f);
    drawList->AddLine(ImVec2(this->Origin2D.x, this->Origin2D.y), ImVec2(this->Z2D.x, this->Z2D.y), IM_COL32(0, 0, 255, 255), 4.0f);
    // 绘制原点到四个角的连线
    drawList->AddLine(ImVec2(this->Origin2D.x, this->Origin2D.y), ImVec2(this->Lens2D[0].x, this->Lens2D[0].y), this->Colour, 4.0f);
    drawList->AddLine(ImVec2(this->Origin2D.x, this->Origin2D.y), ImVec2(this->Lens2D[1].x, this->Lens2D[1].y), this->Colour, 4.0f);
    drawList->AddLine(ImVec2(this->Origin2D.x, this->Origin2D.y), ImVec2(this->Lens2D[2].x, this->Lens2D[2].y), this->Colour, 4.0f);
    drawList->AddLine(ImVec2(this->Origin2D.x, this->Origin2D.y), ImVec2(this->Lens2D[3].x, this->Lens2D[3].y), this->Colour, 4.0f);
    // 绘制镜头面
    drawList->AddLine(ImVec2(this->Lens2D[0].x, this->Lens2D[0].y), ImVec2(this->Lens2D[1].x, this->Lens2D[1].y), this->Colour, 4.0f);
    drawList->AddLine(ImVec2(this->Lens2D[1].x, this->Lens2D[1].y), ImVec2(this->Lens2D[2].x, this->Lens2D[2].y), this->Colour, 4.0f);
    drawList->AddLine(ImVec2(this->Lens2D[2].x, this->Lens2D[2].y), ImVec2(this->Lens2D[3].x, this->Lens2D[3].y), this->Colour, 4.0f);
    drawList->AddLine(ImVec2(this->Lens2D[3].x, this->Lens2D[3].y), ImVec2(this->Lens2D[0].x, this->Lens2D[0].y), this->Colour, 4.0f);
    // 添加文本
    drawList->AddCircleFilled(ImVec2(this->Origin2D.x, this->Origin2D.y), 3.0f, IM_COL32(0, 0, 0, 255));
    if (Label) {
        drawList->AddText(ImVec2(this->Origin2D.x, this->Origin2D.y - 20), IM_COL32(0, 0, 0, 255), Label);
    }
}

void CameraModel::DrawCamera(const DirectX::XMFLOAT3& Position, const DirectX::XMFLOAT3& Rotation, const char* label,
    const float* const pMatrix, const float WinWidth, const float WinHeight) {
    CameraModel Model = *this;
    Model.Rotate(Rotation);
    Model.Move(Position);
    Model.ToScreen(pMatrix, WinWidth, WinHeight);
    Model.Draw(label);
}

void CameraDrawer::Init(float CameraHigh, float CameraX, float CameraY, float AxisLenth, ImU32 Colour) {
    this->CameraModel.Init(CameraHigh, CameraX, CameraY, AxisLenth, Colour);
}

void CameraDrawer::Update(float* pMatrixPtr, float pWinWidth, float pWinHeight) {
    this->pMatrixPtr = pMatrixPtr;
    this->WinWidth = pWinWidth;
    this->WinHeight = pWinHeight;
}

void CameraDrawer::DrawCamera(DirectX::XMFLOAT3 Position, DirectX::XMFLOAT3 Rotation, const char* label) {
    this->CameraModel.DrawCamera(Position, Rotation, label, this->pMatrixPtr, this->WinWidth, this->WinHeight);
}

void CameraDrawer::DrawFrameCamera(const MulNX::Math::Frame& frame, const char* label) {
    auto Rotation = frame.view.rotation;
    auto Position = frame.view.position;
    this->DrawCamera(Position, Rotation, label);
}