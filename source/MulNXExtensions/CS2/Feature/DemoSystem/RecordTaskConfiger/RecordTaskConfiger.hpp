#pragma once
#include <Feature/DemoSystem/DemModuleBase.hpp>

class RecordTaskConfiger final :public DemModuleBase {
    void Window();
    void ProcessMsg(MulNX::Message& msg)override;
    bool Init()override;

    // 所有设置为秒

    // 击杀者视角
    MulReflFloat(preRecordSeconds, 3.5f);
    MulReflFloat(postRecordSeconds, 3.5f);
    // 邻近拼合阈值
    MulReflFloat(mergeThresholdSeconds, 12.0f);
    // 被击杀者视角
    MulReflFloat(preBekilledSeconds, 1.8f);
    MulReflFloat(postBekilledSeconds, 1.8f);
    // 回合后保留秒数
    MulReflFloat(postRoundEndSeconds, 1.0f);
public:
    inline int GetPreRecordTicks()const {
        auto t = this->preRecordSeconds.load(std::memory_order_acquire);
        return static_cast<int>(t * 64.0f);
    }

    inline int GetPostRecordTicks()const {
        auto t = this->postRecordSeconds.load(std::memory_order_acquire);
        return static_cast<int>(t * 64.0f);
    }

    inline int GetMergeThresholdTicks()const {
        auto t = this->mergeThresholdSeconds.load(std::memory_order_acquire);
        return static_cast<int>(t * 64.0f);
    }

    inline int GetPreBekilledTicks()const {
        auto t = this->preBekilledSeconds.load(std::memory_order_acquire);
        return static_cast<int>(t * 64.0f);
    }

    inline int GetPostBekilledTicks()const {
        auto t = this->postBekilledSeconds.load(std::memory_order_acquire);
        return static_cast<int>(t * 64.0f);
    }

    inline int GetPostRoundEndTicks()const {
        auto t = this->postRoundEndSeconds.load(std::memory_order_acquire);
        return static_cast<int>(t * 64.0f);
    }
};