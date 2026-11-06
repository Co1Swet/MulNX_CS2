#pragma once
#include <Intro/CSModuleBase.hpp>
#include <MulNXExtensions/TimeLiner/TimeMixin.hpp>
#include "SubtickCameraTime.hpp"
#include <mutex>

class TimeController final :public CSModuleBase, public TimeAdaptMixin<TimeController> {
    void* pDemoPlayer = nullptr;
    class CSDemoController* pCSDemoController = nullptr;
    float GetMinTime()override;
    float GetMaxTime()override;
    void TimeRend(TimeLiner* timeline, ImDrawList* dl)override;

    bool Init()override;
    void CameraTimeMenu();
    void ProcessMsg(MulNX::Message& msg)override;
    MulReflBool(subtickCameraPath, false);
    std::atomic<uint64_t> cameraTimeGeneration{0};
    std::atomic<MulNX::CameraTimeStatus> cameraTimeStatus{MulNX::CameraTimeStatus::Legacy};
    std::mutex cameraTimeMutex;
    MulNX::ExperimentalCameraTime::Sampler cameraTimeSampler;
public:
    VExecutor<int()> GetDemoTick{};
    VExecutor<bool()> IsPlayingDemo{};
    VExecutor<bool()> IsDemoPaused{};

    int GetDemoLastTick()const;
    float GetTime()override;
    MulNX::CameraPathTime GetCameraPathTime()override;
    bool IsSubtickCameraPathEnabled()const { return subtickCameraPath.load(std::memory_order_acquire); }
    void OnSetOn(float)override { cameraTimeGeneration.fetch_add(1); }
    void OnSetOff()override { cameraTimeGeneration.fetch_add(1); }
    bool SetTime(float time)override;
    bool JumpRealRel(float time);
};
