#pragma once
#include <MulNX/MulNX.hpp>

enum class RecordState : int {
    Free,
    Recording
};

class MediaRunningState final :public MulNX::Module<MediaRunningState> {
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;
public:
    // 该flag主要为了应对多线程音视频系统架构下的同步问题
    // 该标记位使得在录制核心仍忙碌时，位于其它线程上的模块可以安全暂停
    // 该暂停的目的一部分是为了防止队列雪崩导致持续阻塞
    std::atomic<bool> MediaSystemGlobalWorkFlag = false;
    std::atomic<bool> advancedMode = false;

    std::atomic<RecordState> recordState = RecordState::Free;
};