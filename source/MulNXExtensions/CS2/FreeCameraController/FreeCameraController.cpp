#include "FreeCameraController.hpp"
#include <MulNX/Base/UI/UI.hpp>

#include <MulNXExtensions/CS2/ViewController/ViewController.hpp>

void FreeCameraController::Menu(MulNX::UINode* node) {
    bool currentEnable = this->EnableControl.load(std::memory_order_acquire);
    ImGui::Text(I18n("freCamCon.status", currentEnable ? "ON" : "OFF").c_str());

    if (ImGui::Button(I18n("freCamCon.change").c_str())) {
        this->PublishAsync("FreeCamCtrl/Toggle"_hash);
    }

    if (currentEnable) {
        ImGui::Text("当前位置: X=%.2f, Y=%.2f, Z=%.2f",
            this->Position.x, this->Position.y, this->Position.z);

        MulNX::UI::SliderFloat("移动速度", this->MoveSpeed, 10.0f, 1000.0f);
    }
}

bool FreeCameraController::Init() {
    this->kMovUp = this->Shortcut()->GetButton("CamMovUp").value();
    this->kMovDown = this->Shortcut()->GetButton("CamMovDown").value();
    this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {return this->Menu(node);});
    this->SubscribeAsync("FreeCamCtrl/Toggle");
    return true;
}

void FreeCameraController::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "FreeCamCtrl/Toggle"_hash: {
        bool currentEnable = this->EnableControl.load(std::memory_order_acquire);
        if (currentEnable == false) {
            auto view = this->CS2View->GetView();
            // 从未启用到启用：读取当前游戏位置和角度
            this->Position = view.position;
            this->Rotation = view.rotation;
            // 重置自由摄像机独立时钟，避免禁用后再次启用导致时间突变
            this->LastUpdateTime = std::chrono::steady_clock::now();
        }
        this->EnableControl.store(!currentEnable, std::memory_order_release);
        break;
    }
    }
}

bool FreeCameraController::HandleUpdate(CS2::CViewSetup* viewSetup) {
    this->Update();
    if (!this->EnableControl.load(std::memory_order_acquire)) return false;
    this->Rotation = *viewSetup->pViewAngles();

    auto now = std::chrono::steady_clock::now();
    float deltaTime = std::chrono::duration<float>(now - this->LastUpdateTime).count();
    this->LastUpdateTime = now;

    // CS2 坐标轴：X前，Y左，Z上
    DirectX::XMFLOAT3 moveDir = { 0.0f, 0.0f, 0.0f };

    if (this->pInputSystem->IsKeyPressed('W')) moveDir.x += 1.0f;  // 前进
    if (this->pInputSystem->IsKeyPressed('S')) moveDir.x -= 1.0f;  // 后退
    if (this->pInputSystem->IsKeyPressed('A')) moveDir.y += 1.0f;  // 左移
    if (this->pInputSystem->IsKeyPressed('D')) moveDir.y -= 1.0f;  // 右移
    if (this->pInputSystem->IsKeyPackPressed(this->kMovUp)) moveDir.z += 1.0f;  // 上移
    if (this->pInputSystem->IsKeyPackPressed(this->kMovDown)) moveDir.z -= 1.0f;  // 下移

    // 归一化移动方向
    float moveLength = sqrtf(moveDir.x * moveDir.x + moveDir.y * moveDir.y + moveDir.z * moveDir.z);
    if (moveLength > 0.0f) {
        moveDir.x /= moveLength;
        moveDir.y /= moveLength;
        moveDir.z /= moveLength;
    }

    // 使用四元数表示旋转，顺序：先 yaw（Z），再 pitch（Y），再 roll（X）
    // 注：DirectX 的四元数乘法顺序与矩阵一致（右乘表示先应用右侧）
    DirectX::XMVECTOR quatYaw = DirectX::XMQuaternionRotationAxis(DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), DirectX::XMConvertToRadians(this->Rotation.y));
    DirectX::XMVECTOR quatPitch = DirectX::XMQuaternionRotationAxis(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), DirectX::XMConvertToRadians(this->Rotation.x));
    DirectX::XMVECTOR quatRoll = DirectX::XMQuaternionRotationAxis(DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), DirectX::XMConvertToRadians(this->Rotation.z));

    // 组合顺序：先 yaw，再 pitch，再 roll（与矩阵顺序一致）
    DirectX::XMVECTOR quatTemp = DirectX::XMQuaternionMultiply(quatPitch, quatYaw);
    DirectX::XMVECTOR quatRot = DirectX::XMQuaternionMultiply(quatRoll, quatTemp);

    // 从四元数提取三个方向向量
    DirectX::XMFLOAT3 forward, left, up;
    DirectX::XMStoreFloat3(&forward, DirectX::XMVector3Rotate(DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), quatRot));
    // 基向量 (0,1,0) 在本坐标系中表示向左，因此旋转后得到的是 left 向量
    DirectX::XMStoreFloat3(&left, DirectX::XMVector3Rotate(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), quatRot));
    DirectX::XMStoreFloat3(&up, DirectX::XMVector3Rotate(DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), quatRot));

    auto speed = this->MoveSpeed.load(std::memory_order_acquire);
    if (this->pInputSystem->IsKeyPressed(VK_SHIFT)) speed *= 0.2f; // 按住 Shift 减速

    // 应用移动：前/后（moveDir.x）、左/右（moveDir.y，正值为左）、上/下（moveDir.z）
    Position.x += (forward.x * moveDir.x + left.x * moveDir.y + up.x * moveDir.z) * speed * deltaTime;
    Position.y += (forward.y * moveDir.x + left.y * moveDir.y + up.y * moveDir.z) * speed * deltaTime;
    Position.z += (forward.z * moveDir.x + left.z * moveDir.y + up.z * moveDir.z) * speed * deltaTime;

    return true;
}

void FreeCameraController::HandleOverrideView(CS2::CViewSetup* viewSetup) {
    // 如果启用自由摄像机控制，同步角度到InputSystem

    viewSetup->pViewOrigin()->x = this->Position.x;
    viewSetup->pViewOrigin()->y = this->Position.y;
    viewSetup->pViewOrigin()->z = this->Position.z;
}