#pragma once
#include "MediaRunningState/MediaRunningState.hpp"
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
        // 记录在Present线程执行copy时，绝对的世界时间，在常规录制模式多使用
        std::chrono::steady_clock::time_point captureTime{};
        // 一个帧率，在高级录制里使用，用于计算pts
        int frameRate;
        // 一个逻辑帧数字，在高级录制被填充，用于计算pts
        int logicFrame;
        // 需要丢弃该帧
        bool needDrop;
        // 处于高级录制模式;
        bool isAdvancedMode;
    };

    struct AVStartInfo {
        av::FormatContext* pOutCtx = nullptr;
        std::chrono::steady_clock::time_point startTime{};
        bool advancedMode = false;
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