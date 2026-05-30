#pragma once

#include <MulNX/MulNX.hpp>
#include <MulNXThirdParty/queue/concurrentqueue.h>
#include <avcpp/frame.h>

class AudioCapturer final :public MulNX::ModuleBase {
public:
    bool Init()override;
    void Deinit()override;

    std::optional<av::AudioSamples> TryPop();

private:
    moodycamel::ConcurrentQueue<av::AudioSamples> buffer;
    std::atomic<bool> capturing{ false };
    std::unique_ptr<std::thread> captureThread;
};
