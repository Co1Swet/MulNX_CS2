#pragma once

#include "dxmext.hpp"
#include "DOF/DOF.hpp"
#include "Translate/Translate.hpp"

#include <atomic>
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

        class ViewBuffer {
            MulNX::Math::View view;
            bool initialized = false;
        public:
            // 平滑系数
            std::atomic<float> SMOOTH_FACTOR{ 0.2f };
            void Push(MulNX::Math::View newView);
            MulNX::Math::View& Get() { return this->view; }
        };

        // 包含三个点，用于构建坐标系
        class Point3 {
        public:
            DirectX::XMFLOAT3 origin;
            DirectX::XMFLOAT3 forward;
            DirectX::XMFLOAT3 up;
        };

        // 帧，包含渲染一帧的所有预备前置条件
        class Frame {
        public:
            // 目标空间状态，自由摄像机轨道完全使用，锁定摄像机轨道使用视角，不使用其它
            View view{};

            // 目标OB模式，4是自由摄像机轨道和锁定摄像机轨道，2是第一人称摄像机轨道
            uint8_t TargetOBMode = 4;
            // 第一人称摄像机轨道和锁定摄像机轨道使用，均指代目标人物
            uint8_t TargetPlayerIndexInMap = 0;

            float GameSpeed{};

            bool operator==(const Frame&)const = default;
        };

        // 关键帧,是时间、空间状态、FOV、景深的组合
        class CameraKeyframe {
        public:
            DirectX::XMVECTOR PositionAndFOV{};
            DirectX::XMVECTOR RotationQuat{};
            DirectX::XMVECTOR dof{};

            float time{};

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