#pragma once
#include <Intro/CSModuleBase.hpp>
#include <Buildup/ParticleManager/ParticleManager.hpp>
#include <Buildup/ParticleSystemMgr/ParticleSystemMgr.hpp>

class WorldDraw final :public CSModuleBase {
    ParticleManager* pParticleManager = nullptr;
    ParticleSystemMgr* pParticleSystemMgr = nullptr;

    struct Vector3 {
        float x, y, z;
    };

    struct ParticleTrail {
        static constexpr size_t MAX_POINTS = 1024;

        int particleIndex = -1;
        int64_t trailHandle = 0;
        // 位置历史缓冲区（循环使用）
        std::vector<Vector3> positions;
        std::vector<float> times; 

        ParticleTrail() {
            positions.reserve(MAX_POINTS);
            times.reserve(MAX_POINTS);
        }
    };

    float m_flTrailBaseTime = 0.0f;   // 创建粒子的基准时间
    float m_flLastSampleTime = 0.0f;  // 上次采样时间
    const float m_flSampleInterval = 0.05f; // 采样间隔（秒）

    ParticleTrail myTrail{};

    bool first = true;

    bool Init()override;
    void OnFirst();
    void UpdateMyDraw();
};