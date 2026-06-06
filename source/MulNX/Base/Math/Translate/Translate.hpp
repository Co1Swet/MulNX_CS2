#pragma once

#include <MulNX/Base/Math/dxmext.hpp>

namespace MulNX {
    namespace Math {
        void CSEulerToQuat(const DirectX::XMFLOAT3& Euler, DirectX::XMFLOAT4& QuaT);
        DirectX::XMVECTOR CSEulerToQuatVec(const DirectX::XMFLOAT3& Euler);
        void CSQuatToEuler(const DirectX::XMFLOAT4& Quat, DirectX::XMFLOAT3& Euler);
        // 只修改Pitch和Yaw，不修改Roll
        void CSDirToEuler(const DirectX::XMFLOAT3& Dir, DirectX::XMFLOAT3& Euler);
        // 从欧拉角（俯仰、偏航）计算方向向量（单位向量）
        DirectX::XMFLOAT3 CSEulerToDir(float pitchDegrees, float yawDegrees);


        bool WorldToScreen(const DirectX::XMFLOAT3& pWorldPos, DirectX::XMFLOAT2& pScreenPos, const float* pMatrixPtr, const float pWinWidth, const float pWinHeight);

        DirectX::XMFLOAT3 RotatePoint(
            const DirectX::XMFLOAT3& inputPoint,
            float pitchDegrees,  // 绕Y轴旋转（俯仰）
            float yawDegrees,    // 绕Z轴旋转（偏航）
            float rollDegrees    // 绕X轴旋转（滚转）
        );

        //移动点
        bool MovePoint(DirectX::XMFLOAT3& SourcePoint, const DirectX::XMFLOAT3& TargetPoint);

        bool BuildLocalCoordinateSystem(
            const DirectX::XMFLOAT3& origin,
            const DirectX::XMFLOAT3& forwardPoint,
            const DirectX::XMFLOAT3& upPoint,
            DirectX::XMFLOAT3& forward,
            DirectX::XMFLOAT3& left,
            DirectX::XMFLOAT3& up,
            bool invertUp = false
        );
    };
};