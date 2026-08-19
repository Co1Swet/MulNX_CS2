#pragma once
#include "MediaRecorder/MediaRunningState/MediaRunningState.hpp"
#include <MulNXExtensions/GraphicsManager/GraphicsManager.hpp>
#include <wrl/client.h>
#include <d3d11.h>
#include <avcpp/av.h>
#include <avcpp/format.h>
#include <avcpp/codec.h>
#include <avcpp/frame.h>
#include <avcpp/packet.h>
#include <avcpp/codeccontext.h>
#include <avcpp/formatcontext.h>
#include <avcpp/videorescaler.h>
#include <avcpp/audioresampler.h>

namespace MulNX {
    class VFrameExInfo {
    public:
        // 记录在Present线程执行copy时，自然的挂钟时间，或高级录制下赋予Virtual Time
        std::chrono::steady_clock::time_point captureTime{};
        // 需要丢弃该帧
        bool needDrop;
        // 处于高级录制模式;
        bool isAdvancedMode;
    };

    struct AVStartInfo {
        av::FormatContext* pOutCtx = nullptr;
        std::chrono::steady_clock::time_point startTime{};
    };
}

template <typename T>
class MediaModuleMixin {
    T* This() { return static_cast<T*>(this); }
protected:
    MediaRunningState* pMediaState = nullptr;
    MediaModuleMixin() {
        This()->preInits.push_back([this]() -> bool {
            this->pMediaState = This()->FindModule<MediaRunningState>("MediaRunningState");
            return true;
            });
    }
};

class MediaModuleBase :public MulNX::Module<MediaModuleBase>, public MediaModuleMixin<MediaModuleBase> {};
using Microsoft::WRL::ComPtr;