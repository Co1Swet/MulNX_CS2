#include "Translate.hpp"

#include <algorithm>
#include <cmath>

void MulNX::Math::CSEulerToQuat(const DirectX::XMFLOAT3& Euler, DirectX::XMFLOAT4& QuaT) {
    auto quatResult = CSEulerToQuatVec(Euler);
    DirectX::XMStoreFloat4(&QuaT, quatResult);
    return;
}

DirectX::XMVECTOR MulNX::Math::CSEulerToQuatVec(const DirectX::XMFLOAT3& Euler) {
    // 将角度转换为弧度
    float pitchRad = DirectX::XMConvertToRadians(Euler.x); // 俯仰角（绕Y轴）
    float yawRad = DirectX::XMConvertToRadians(Euler.y);   // 偏航角（绕Z轴）
    float rollRad = DirectX::XMConvertToRadians(Euler.z);  // 滚转角（绕X轴）

    // 创建旋转轴
    DirectX::XMVECTOR axisZ = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    DirectX::XMVECTOR axisY = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    DirectX::XMVECTOR axisX = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);

    // 计算各旋转的四元数
    DirectX::XMVECTOR quatYaw = DirectX::XMQuaternionRotationAxis(axisZ, yawRad);
    DirectX::XMVECTOR quatPitch = DirectX::XMQuaternionRotationAxis(axisY, pitchRad);
    DirectX::XMVECTOR quatRoll = DirectX::XMQuaternionRotationAxis(axisX, rollRad);

    // 按顺序组合旋转：先偏航，再俯仰，最后滚转
    DirectX::XMVECTOR quatTemp = DirectX::XMQuaternionMultiply(quatPitch, quatYaw);
    DirectX::XMVECTOR quatResult = DirectX::XMQuaternionMultiply(quatRoll, quatTemp);

    return quatResult;
}

void MulNX::Math::CSQuatToEuler(const DirectX::XMFLOAT4& Quat, DirectX::XMFLOAT3& Euler) {
    //提取四元数分量
    const float& x = Quat.x;
    const float& y = Quat.y;
    const float& z = Quat.z;
    const float& w = Quat.w;

    //计算偏航角（yaw）
    float yawRad = std::atan2(2.0f * (w * z + x * y), 1.0f - 2.0f * (y * y + z * z));

    //计算中间值用于俯仰角和万向锁检测
    float test = 2.0f * (w * y - z * x);
    const float threshold = 0.9999999f; // 万向锁阈值

    float pitchRad, rollRad;

    //处理万向锁（俯仰角接近±90度）
    if (test > threshold) {
        pitchRad = -DirectX::XM_PIDIV2; // -90度（向上）
        rollRad = 0.0f;
    }
    else if (test < -threshold) {
        pitchRad = DirectX::XM_PIDIV2;  // +90度（向下）
        rollRad = 0.0f;
    }
    else {
        //计算俯仰角和滚转角
        float pitchStd = std::asin(test);
        float rollStd = std::atan2(2.0f * (w * x + y * z), 1.0f - 2.0f * (x * x + y * y));

        //转换旋转方向
        pitchRad = pitchStd; // 俯仰角取负
        rollRad = rollStd;   // 滚转角取负
    }

    //将弧度转换为角度
    Euler.x = DirectX::XMConvertToDegrees(pitchRad); // 俯仰角
    Euler.y = DirectX::XMConvertToDegrees(yawRad);   // 偏航角
    Euler.z = DirectX::XMConvertToDegrees(rollRad);  // 滚转角

    return;
}

void MulNX::Math::CSDirToEuler(const DirectX::XMFLOAT3& Dir, DirectX::XMFLOAT3& Euler) {
    Euler.x = std::atan2(-Dir.z, std::sqrt(Dir.x * Dir.x + Dir.y * Dir.y)) * (180.0 / DirectX::XM_PI);
    Euler.y = std::atan2(Dir.y, Dir.x) * (180.0 / DirectX::XM_PI);

    Euler.x = (Euler.x < -89.0f) ? -89.0f : Euler.x;
    Euler.x = (Euler.x > 89.f) ? 89.0f : Euler.x;

    while (Euler.y > 180.f)Euler.y -= 360.f;
    while (Euler.y < -180.f)Euler.y += 360.f;
}
DirectX::XMFLOAT3 MulNX::Math::CSEulerToDir(float pitchDegrees, float yawDegrees) {
    float pitchRad = pitchDegrees * (DirectX::XM_PI / 180.0f);
    float yawRad = yawDegrees * (DirectX::XM_PI / 180.0f);

    float cosPitch = std::cos(pitchRad);
    float sinPitch = std::sin(pitchRad);
    float cosYaw = std::cos(yawRad);
    float sinYaw = std::sin(yawRad);

    return DirectX::XMFLOAT3(
        cosPitch * cosYaw,
        cosPitch * sinYaw,
        -sinPitch
    );
}

bool MulNX::Math::WorldToScreen(const DirectX::XMFLOAT3& pWorldPos, DirectX::XMFLOAT2& pScreenPos, const float* pMatrixPtr, const float pWinWidth, const float pWinHeight) {
    if (!pMatrixPtr) return false;

    const float mX{ pWinWidth / 2 };
    const float mY{ pWinHeight / 2 };

    // 直接使用原始矩阵指针，通过索引访问
    const float w{
        pMatrixPtr[12] * pWorldPos.x +    // matrix[3][0]
        pMatrixPtr[13] * pWorldPos.y +    // matrix[3][1] 
        pMatrixPtr[14] * pWorldPos.z +    // matrix[3][2]
        pMatrixPtr[15] };                 // matrix[3][3]

    if (w <= 0.0f) return false;

    const float x{
        pMatrixPtr[0] * pWorldPos.x +     // matrix[0][0]
        pMatrixPtr[1] * pWorldPos.y +     // matrix[0][1]
        pMatrixPtr[2] * pWorldPos.z +     // matrix[0][2]
        pMatrixPtr[3] };                  // matrix[0][3]

    const float y{
        pMatrixPtr[4] * pWorldPos.x +     // matrix[1][0]
        pMatrixPtr[5] * pWorldPos.y +     // matrix[1][1]
        pMatrixPtr[6] * pWorldPos.z +     // matrix[1][2]
        pMatrixPtr[7] };                  // matrix[1][3]

    pScreenPos.x = (mX + mX * x / w);
    pScreenPos.y = (mY - mY * y / w);

    return true;
}

bool MulNX::Math::BuildLocalCoordinateSystem(
    const DirectX::XMFLOAT3& origin,
    const DirectX::XMFLOAT3& forwardPoint,
    const DirectX::XMFLOAT3& upPoint,
    DirectX::XMFLOAT3& forward,
    DirectX::XMFLOAT3& left,
    DirectX::XMFLOAT3& up,
    bool invertUp) {

    // 1. 前向轴：原点 → 前向点
    forward = forwardPoint - origin;
    float len = sqrtf(forward.x * forward.x + forward.y * forward.y + forward.z * forward.z);
    if (len < 0.0001f) return false;
    forward.x /= len; forward.y /= len; forward.z /= len;

    // 2. 上向候选轴：原点 → 上点
    DirectX::XMFLOAT3 upCandidate = upPoint - origin;
    len = sqrtf(upCandidate.x * upCandidate.x + upCandidate.y * upCandidate.y + upCandidate.z * upCandidate.z);
    if (len < 0.0001f) return false;
    upCandidate.x /= len; upCandidate.y /= len; upCandidate.z /= len;
    if (invertUp) {
        upCandidate.x = -upCandidate.x;
        upCandidate.y = -upCandidate.y;
        upCandidate.z = -upCandidate.z;
    }

    // 3. 使用叉乘计算左向量：left = cross(upCandidate, forward) 归一化
    DirectX::XMVECTOR f = DirectX::XMLoadFloat3(&forward);
    DirectX::XMVECTOR uc = DirectX::XMLoadFloat3(&upCandidate);
    DirectX::XMVECTOR leftVec = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(uc, f)); // 注意顺序：up × forward

    float leftLenSq = 0.0f;
    DirectX::XMStoreFloat(&leftLenSq, DirectX::XMVector3LengthSq(leftVec));
    if (leftLenSq < 0.0001f) return false;

    DirectX::XMStoreFloat3(&left, leftVec);

    // 4. 重新正交化上轴：up = cross(forward, left) 归一化
    DirectX::XMVECTOR upVec = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(f, leftVec));
    DirectX::XMStoreFloat3(&up, upVec);

    return true;
}

DirectX::XMFLOAT3 MulNX::Math::RotatePoint(
    const DirectX::XMFLOAT3& inputPoint,
    float pitchDegrees,  // 绕Y轴旋转（俯仰）
    float yawDegrees,    // 绕Z轴旋转（偏航）
    float rollDegrees    // 绕X轴旋转（滚转）
) {
    // 直接使用已定义的欧拉角到四元数转换，保持与其它数学假设一致。
    DirectX::XMVECTOR rotationQuaternion = CSEulerToQuatVec({ pitchDegrees, yawDegrees, rollDegrees });
    rotationQuaternion = DirectX::XMQuaternionNormalize(rotationQuaternion);

    DirectX::XMVECTOR point = DirectX::XMLoadFloat3(&inputPoint);
    DirectX::XMVECTOR rotatedVector = DirectX::XMVector3Rotate(point, rotationQuaternion);

    DirectX::XMFLOAT3 result;
    DirectX::XMStoreFloat3(&result, rotatedVector);
    return result;
}

bool MulNX::Math::MovePoint(DirectX::XMFLOAT3& SourcePoint, const DirectX::XMFLOAT3& TargetPoint) {
    SourcePoint += TargetPoint;
    return true;
}