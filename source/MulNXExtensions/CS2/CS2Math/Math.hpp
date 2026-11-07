#pragma once
#include "dxmext.hpp"
#include "DOF/DOF.hpp"
#include "Translate/Translate.hpp"
#include "Draw/Draw.hpp"

namespace MulNX {
    namespace Math {
        // 包含游戏提取视角信息
        class View {
        public:
            DirectX::XMFLOAT3 position;
            DirectX::XMFLOAT3 rotation;
            float FOV;
            DOFParam dof;

            DirectX::XMVECTOR ToPositionAndFOV();
            DirectX::XMVECTOR ToRotationQuat();
            DirectX::XMVECTOR ToDOFPack();
        };

        // 包含三个点，用于构建坐标系
        class Point3 {
        public:
            DirectX::XMFLOAT3 origin;
            DirectX::XMFLOAT3 forward;
            DirectX::XMFLOAT3 up;
        };

        // 关键帧,是时间、空间状态、FOV、景深的组合
        class CameraKeyframe {
        public:
            DirectX::XMVECTOR PositionAndFOV{};
            DirectX::XMVECTOR RotationQuat{};
            DirectX::XMVECTOR dof{};

            double time = 0.0f;

            DirectX::XMFLOAT3 GetPosition()const;
            DirectX::XMFLOAT4 GetRotationQuat()const;
            DirectX::XMFLOAT3 GetRotationEuler()const;
            DirectX::XMFLOAT4 GetPositionAndFOV()const;
            DOFParam GetDOF()const;
            float GetFOV()const;
            std::string GetMsg()const;
        };
    };
};