#include "AdvancedViewController.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <Buildup/TimeController/TimeController.hpp>
#include <Intro/HookView/HookView.hpp>

bool AdvancedViewController::Menu() {
    if (ImGui::CollapsingHeader("高级视角控制")) {
        MulNX::UI::Checkbox("启用高级视角控制", this->runFlag1);
        MulNX::UI::Checkbox("覆盖自视角", this->OverrideSelfView);
        MulNX::UI::Checkbox("始终计算视角", this->AlwaysCaulate);

        MulNX::UI::Checkbox("使用本地Pawn", this->useLocalPawn);
        MulNX::UI::Checkbox("强制头模式", this->forceHeadMode);
        // 骨骼来源模式：武器 / 人体
        MulNX::UI::Checkbox("人体骨骼模式", this->UseBodyBones);

        // 骨骼选择（三点定义坐标系）
        MulNX::UI::SliderInt("骨骼起点 (原点)", this->boneIndex1, 0, 127);
        MulNX::UI::SliderInt("骨骼终点 (前向)", this->boneIndex2, 0, 127);
        MulNX::UI::SliderInt("骨骼辅助 (上向参考)", this->boneIndex3, 0, 127);

        // 位置偏移（前, 左, 上） — 与输入系统约定对应：localPositionOffset (x=前, y=左, z=上)
        ImGui::Text("摄像机局部偏移 (前,左,上)");
        ImGui::SliderFloat("前移 (X)", &this->localPositionOffset.x, -200.0f, 200.0f);
        ImGui::SliderFloat("左移 (Y)", &this->localPositionOffset.y, -200.0f, 200.0f);
        ImGui::SliderFloat("上移 (Z)", &this->localPositionOffset.z, -200.0f, 200.0f);

        // 旋转偏移（统一为：俯仰, 偏航, 滚转）
        ImGui::Text("摄像机局部旋转 (俯仰,偏航,滚转)");
        ImGui::SliderFloat("俯仰 (Pitch)", &this->localRotationOffset.x, -89.0f, 89.0f);
        ImGui::SliderFloat("偏航 (Yaw)", &this->localRotationOffset.y, -180.0f, 180.0f);
        ImGui::SliderFloat("滚转 (Roll)", &this->localRotationOffset.z, -180.0f, 180.0f);

        // 平滑系数
        MulNX::UI::SliderFloat("平滑系数", this->viewBuffer.SMOOTH_FACTOR, 0.0f, 1.0f);

        // 上向反转选项
        MulNX::UI::Checkbox("反转上向", this->InvertUp);
        // 绘制选项：原始骨骼点与坐标轴
        MulNX::UI::Checkbox("显示原始骨骼点", this->ShowOriginalBones);
        MulNX::UI::Checkbox("显示坐标轴", this->ShowCoordinateAxes);
        MulNX::UI::SliderFloat("坐标轴长度", this->AxisLength, 1.0f, 200.0f);

        // 绘制当前骨骼位置（调试用）
        if (this->hasAxisInfo.load(std::memory_order_acquire)) {
            auto read = this->currentAxisInfo.Read();
            MulNX::TransInfo info;
            info.pMatrix = this->CS2View->GetViewMatrix();
            info.windowHeight = this->CS2View->GetWinHeight();
            info.windowWidth = this->CS2View->GetWinWidth();

            // 分两部分绘制：原始骨骼点 与 坐标轴
            if (this->ShowOriginalBones.load(std::memory_order_acquire)) {
                MulNX::UI::DrawWorldPoint(read->PosOrigin, info, "Origin");
                MulNX::UI::DrawWorldPoint(read->PosForward, info, "Forward");
                MulNX::UI::DrawWorldPoint(read->PosUp, info, "Up");
            }

            if (this->ShowCoordinateAxes.load(std::memory_order_acquire)) {
                float axisLen = this->AxisLength.load(std::memory_order_acquire);
                DirectX::XMFLOAT3 org = read->PosOrigin;
                DirectX::XMFLOAT3 f_end = (read->AxisForward * axisLen) + org;
                DirectX::XMFLOAT3 u_end = (read->AxisUp * axisLen) + org;
                DirectX::XMFLOAT3 r_end = (read->AxisLeft * axisLen) + org;

                MulNX::UI::DrawWorldLine(org, f_end, info, ImColor(255, 0, 0), 2.0f);
                MulNX::UI::DrawWorldLine(org, u_end, info, ImColor(0, 255, 0), 2.0f);
                MulNX::UI::DrawWorldLine(org, r_end, info, ImColor(0, 0, 255), 2.0f);

                // 在轴端点绘制点与标签
                MulNX::UI::DrawWorldPoint(f_end, info, "F");
                MulNX::UI::DrawWorldPoint(u_end, info, "U");
                MulNX::UI::DrawWorldPoint(r_end, info, "L");
            }
        }
    }

    return true;
}

bool AdvancedViewController::Init() {
    this->SendUINode(this->GetName(), [this](auto&&...) {return this->Menu(); });

    return true;
}

bool AdvancedViewController::HandleUpdate(CS2::CViewSetup* viewSetup, const int& num) {
    // 如果启用了高级视角控制，尝试更新视角数据，注意这里只是更新，不涉及具体的视角控制逻辑，视角控制逻辑在后面根据状态分流执行
    if (!this->runFlag1.load(std::memory_order_acquire)) return false;

    // 通过时间桥判断是否需要更新视角，防止抖动
    static auto lastTime = this->CS2Time->GetReal();
    auto currentTime = this->CS2Time->GetReal();
    if (currentTime > lastTime || lastTime - currentTime > 0.015f || this->AlwaysCaulate.load(std::memory_order_acquire)) {
        auto result = this->HandleSelfViewUpdate(viewSetup);
        if (result.has_value()) {
            auto newView = result.value();
            this->viewBuffer.Push(newView);
        }
        else {
            this->hasAxisInfo.store(false, std::memory_order_release);
            this->LogWarning(std::format("HandleSelfViewUpdate failed with code: 0x{:X}", result.error()));
        }
        lastTime = currentTime;
    }

    return this->HandleOverrideView(viewSetup);
}

std::expected<MulNX::Math::Point3, int> AdvancedViewController::GetPoint3(CS2::CViewSetup* viewSetup) {
    // 如果是头模式，那么根据viewSetup的位置和角度计算出一个假想的头部位置，并且基于欧拉角，给出恰好的三点
    if (this->forceHeadMode.load(std::memory_order_acquire)) {
        MulNX::Math::Point3 point3{};

        // 使用 viewSetup 的位置和角度来构造一个假想的头部三点：
        // - origin: 摄像机位置（视点）
        // - forward: origin 沿视向方向的一点
        // - up: origin 沿上方向的一点

        // 读取视点位置与角度（角度按 x=pitch, y=yaw, z=roll）
        DirectX::XMFLOAT3 camPos = *viewSetup->pViewOrigin();
        DirectX::XMFLOAT3 ang = *viewSetup->pViewAngles();

        // 角度转弧度（保持与工程中其它位置使用的顺序一致：yaw->pitch->roll）
        float pitchRad = ang.x * (DirectX::XM_PI / 180.0f);
        float yawRad = ang.y * (DirectX::XM_PI / 180.0f);
        float rollRad = ang.z * (DirectX::XM_PI / 180.0f);

        // 使用与其它代码一致的轴：yaw 绕 Z, pitch 绕 Y, roll 绕 X
        DirectX::XMVECTOR quatYaw = DirectX::XMQuaternionRotationAxis(DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), yawRad);
        DirectX::XMVECTOR quatPitch = DirectX::XMQuaternionRotationAxis(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), pitchRad);
        DirectX::XMVECTOR quatRoll = DirectX::XMQuaternionRotationAxis(DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), rollRad);

        DirectX::XMVECTOR quatTemp = DirectX::XMQuaternionMultiply(quatPitch, quatYaw);
        DirectX::XMVECTOR quatView = DirectX::XMQuaternionMultiply(quatRoll, quatTemp);
        DirectX::XMMATRIX rotView = DirectX::XMMatrixRotationQuaternion(quatView);

        // 局部基向量（注意：本代码将 X 视为 forward, Z 视为 up）
        DirectX::XMVECTOR vForward = DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), rotView);
        DirectX::XMVECTOR vUp = DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotView);

        DirectX::XMFLOAT3 fwd; DirectX::XMFLOAT3 up;
        DirectX::XMStoreFloat3(&fwd, vForward);
        DirectX::XMStoreFloat3(&up, vUp);

        // 选取合适的参考长度（只要不为 0 即可），用 16 单位作为默认尺度
        const float refLen = 16.0f;

        point3.origin = camPos;
        point3.forward = (fwd * refLen) + camPos;
        point3.up = (up * refLen) + camPos;

        return point3;
    }

    try {
        auto* target = this->GetSelfViewTargetPawn();
        if (!target) return std::unexpected(3);

        // 根据模式选择骨骼来源（武器或人体），二选一（不回退）
        MulNX::Math::Point3 point3{};
        if (this->UseBodyBones.load(std::memory_order_acquire)) {
            // 从人体读取骨骼
            point3.origin = target->GetBonePos(this->boneIndex1.load());
            point3.forward = target->GetBonePos(this->boneIndex2.load());
            point3.up = target->GetBonePos(this->boneIndex3.load());
        }
        else {
            // 从武器读取骨骼
            auto hActiveWeapon = target->GetHandleActiveWeapon();
            auto* pWeapon = this->CS2->client.GetBaseEntityFromHandle(hActiveWeapon)->As<CS2::C_BasePlayerWeapon>();
            if (!pWeapon) return std::unexpected(4);
            point3.origin = pWeapon->GetBonePos(this->boneIndex1.load());
            point3.forward = pWeapon->GetBonePos(this->boneIndex2.load());
            point3.up = pWeapon->GetBonePos(this->boneIndex3.load());
        }
        return point3;
    }
    catch (const std::exception& e) {
        this->LogWarning(std::format("GetObserverTargetBonePos exception: {}", e.what()));
        return std::unexpected(5);
    }
}

CS2::C_CSPlayerPawn* AdvancedViewController::GetSelfViewTargetPawn() {
    if (this->useLocalPawn.load(std::memory_order_acquire)) {
        return this->CS2->client.GetLocalPlayerPawn();
    }
    return this->CS2->client.TryGetObservingPawn();
}

std::expected<MulNX::Math::View, int> AdvancedViewController::HandleSelfViewUpdate(CS2::CViewSetup* viewSetup) {
    try {
        auto point3Result = this->GetPoint3(viewSetup);
        if (!point3Result.has_value()) {
            return std::unexpected(point3Result.error());
        }
        auto point3 = point3Result.value();

        // ========== 构建局部坐标系 ==========
        DirectX::XMFLOAT3 forward, left, up;
        if (!MulNX::Math::BuildLocalCoordinateSystem(point3.origin, point3.forward, point3.up,
            forward, left, up,
            this->InvertUp.load(std::memory_order_acquire))) {
            return std::unexpected(5);
        }

        // 存储调试信息
        {
            auto write = this->currentAxisInfo.Write();
            write->PosOrigin = point3.origin;
            write->PosForward = point3.forward;
            write->PosUp = point3.up;
            write->AxisForward = forward;
            write->AxisLeft = left;
            write->AxisUp = up;

            this->hasAxisInfo.store(true, std::memory_order_release);
        }
        // ========== 构建局部坐标系到世界坐标系的旋转矩阵 ==========
        // 注意：DirectX 使用行主序矩阵，矩阵的行（而非列）应表示局部轴在世界坐标系中的分量
        // 所以每一行分别是 X 轴(forward)、Y 轴(left)、Z 轴(up)
        DirectX::XMMATRIX rotLocalToWorld = DirectX::XMMatrixSet(
            forward.x, forward.y, forward.z, 0.0f,    // 行0: X轴(forward)
            left.x, left.y, left.z, 0.0f,    // 行1: Y轴(left)
            up.x, up.y, up.z, 0.0f,    // 行2: Z轴(up)
            0.0f, 0.0f, 0.0f, 1.0f
        );

        // ========== 计算摄像机世界位置 ==========
        // 将局部偏移变换到世界坐标系
        DirectX::XMVECTOR localOffset = DirectX::XMLoadFloat3(&this->localPositionOffset);

        // 位置去耦：直接使用局部坐标系到世界坐标系的变换，
        // 使 localRotationOffset 不影响最终位置（旋转仅用于方向）
        DirectX::XMVECTOR worldOffset = DirectX::XMVector3TransformCoord(localOffset, rotLocalToWorld);

        DirectX::XMFLOAT3 worldOffsetVec;
        DirectX::XMStoreFloat3(&worldOffsetVec, worldOffset);

        // 计算最终摄像机位置
        DirectX::XMFLOAT3 targetCameraPos = point3.origin + worldOffsetVec;

        // ========== 计算摄像机世界旋转 ==========
        // 构建局部旋转偏移矩阵
        // 注意：旋转顺序应该是Yaw→Pitch→Roll
        float pitchRad = this->localRotationOffset.x * (DirectX::XM_PI / 180.0f);
        float yawRad = this->localRotationOffset.y * (DirectX::XM_PI / 180.0f);
        float rollRad = this->localRotationOffset.z * (DirectX::XM_PI / 180.0f);

        // 创建旋转四元数
        DirectX::XMVECTOR quatYaw = DirectX::XMQuaternionRotationAxis(DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), yawRad);
        DirectX::XMVECTOR quatPitch = DirectX::XMQuaternionRotationAxis(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), pitchRad);
        DirectX::XMVECTOR quatRoll = DirectX::XMQuaternionRotationAxis(DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), rollRad);

        // 组合顺序
        DirectX::XMVECTOR quatTemp = DirectX::XMQuaternionMultiply(quatPitch, quatYaw);
        DirectX::XMVECTOR quatLocalRot = DirectX::XMQuaternionMultiply(quatRoll, quatTemp);

        // 将四元数转换为矩阵
        DirectX::XMMATRIX rotLocalOffset = DirectX::XMMatrixRotationQuaternion(quatLocalRot);

        // 计算最终旋转矩阵
        // 先应用局部旋转偏移，再变换到世界坐标系
        DirectX::XMMATRIX rotFinal = DirectX::XMMatrixMultiply(rotLocalOffset, rotLocalToWorld);

        // 从最终旋转矩阵中提取四元数
        DirectX::XMVECTOR quatFinal = DirectX::XMQuaternionRotationMatrix(rotFinal);
        DirectX::XMFLOAT4 quatF;
        DirectX::XMStoreFloat4(&quatF, quatFinal);

        // 将四元数转换为欧拉角
        DirectX::XMFLOAT3 targetCameraAngle;
        MulNX::Math::CSQuatToEuler(quatF, targetCameraAngle);

        // 规范化角度
        while (targetCameraAngle.y > 180.0f) targetCameraAngle.y -= 360.0f;
        while (targetCameraAngle.y < -180.0f) targetCameraAngle.y += 360.0f;

        return std::expected<MulNX::Math::View, int>(MulNX::Math::View(targetCameraPos, targetCameraAngle, 90.0f));
    }
    catch (...) {
        return std::unexpected(0xFFFF);
    }
}

bool AdvancedViewController::HandleOverrideView(CS2::CViewSetup* viewSetup) {
    if (!this->OverrideSelfView.load(std::memory_order_acquire))return false;
    // 输出平滑后的值
    *viewSetup->pViewOrigin() = this->viewBuffer.Get().position;
    *viewSetup->pViewAngles() = this->viewBuffer.Get().rotation;
    return true;
}