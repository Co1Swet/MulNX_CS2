#include "AdTimeFix.hpp"

bool AdTimeFix::Init() {
    this->pThis = this;

    this->SubscribeSync("Hook/LoadLibraryExW/scenesystem.dll", [this](MulNX::Message& msg) {
        auto slot = this->CS2->scenesystem.FindIatSlot("tier0.dll", "Plat_FloatTime");
        this->hkScenesystem_Plat_FloatTime = MulNX::Memory::IatHook(slot, (void*)+[](void)->double {
            return pThis->OnGetTime();
            });
        if (!this->pRawPlat_FloatTime) {
            this->pRawPlat_FloatTime = this->hkScenesystem_Plat_FloatTime.Original<Plat_FloatTime_t>();
        }
        this->RegisterAttachIatHook(this->hkScenesystem_Plat_FloatTime, "Plat_FloatTime(Scenesystem)");
        });

    this->SubscribeSync("Hook/LoadLibraryExW/panorama.dll", [this](auto&&...) {
        auto slot = this->CS2->panorama.FindIatSlot("tier0.dll", "Plat_FloatTime");
        this->hkPanorama_Plat_FloatTime = MulNX::Memory::IatHook(slot, (void*)+[](void)->double {
            return pThis->OnGetTime();
            });
        if (!this->pRawPlat_FloatTime) {
            this->pRawPlat_FloatTime = this->hkPanorama_Plat_FloatTime.Original<Plat_FloatTime_t>();
        }
        this->RegisterAttachIatHook(this->hkPanorama_Plat_FloatTime, "Plat_FloatTime(Panorama)");
        });

    return true;
}

double AdTimeFix::OnGetTime(void) {
    std::lock_guard<std::mutex> lock(m_mutex);
    const double raw = this->pRawPlat_FloatTime();

    // 初始化：虚拟时间 = raw
    if (!m_initialized) {
        m_initialized = true;
        m_lastResult = raw;
        m_lastTime = raw;
        m_lastFrameCount = 0;
        m_frameTime = 0.0;
        return m_lastResult;
    }
    double delta = raw - m_lastTime;
    auto Fail = [&]()->double {
        // 加载 / 未初始化：直接跟随 raw
        m_lastResult += m_frameTime + delta;
        m_frameTime = 0.0;
        m_lastTime = raw;
        m_lastFrameCount = 0;
        return m_lastResult;
        };

    int frame_count = 0;
    double frametime = 0.0f;
    try {
        // 计算自上次基准以来的 raw 增量
        auto gv = this->CS2->CSGlobalVars;
        if (!gv)return Fail();
        frame_count = MulNX::MRead(&gv->framecount);
        frametime = MulNX::MRead(&gv->frametime);
    }
    catch (const MulNX::Exception& e) {
        this->LogWarning(e);
        return Fail();
    }


    // 始终生效的平滑分支
    if (0 != frame_count) {
        // 帧边界：catch-up + 重置基准
        if (frame_count != m_lastFrameCount) {
            m_lastResult += m_frameTime;
            m_frameTime = frametime;
            m_lastTime = raw;
            m_lastFrameCount = frame_count;
        }

        // 帧内平滑推进
        const double ratio = std::min(
            std::max(0.0, m_frameTime ? delta / m_frameTime : 0.0),
            1.0
        );
        const double useTime = ratio * m_frameTime * 0.5;
        m_frameTime -= useTime;
        m_lastResult += useTime;
    }
    else return Fail();

    return m_lastResult;
}