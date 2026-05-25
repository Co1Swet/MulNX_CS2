#include "MediaProcesser.hpp"
#include <avcpp/av.h>
#include <avcpp/codec.h>
#include <avcpp/averror.h>

bool MediaProcesser::Init() {
    try {
        // 1. 初始化 FFmpeg 所有组件
        av::init();
        av::set_logging_level(AV_LOG_WARNING);  // 减少 FFmpeg 自身输出

        // 2. 尝试验证一个必然存在的解码器（H.264 软件解码器在 LGPL 版本中内置）
        auto codec = av::findDecodingCodec(AV_CODEC_ID_H264);
        if (codec.isNull()) {
            this->ISys().LogError("[MediaProcesser] H.264 decoder not found – "
                "FFmpeg DLLs may be missing or incomplete.");
            return false;
        }

        this->ISys().LogSucc("[MediaProcesser] FFmpeg libraries loaded successfully.");
        return true;
    }
    catch (const av::Exception& e) {
        this->ISys().LogError("[MediaProcesser] FFmpeg initialization failed: " + std::string(e.what()));
        return false;
    }
    catch (const std::exception& e) {
        this->ISys().LogError("[MediaProcesser] Unexpected error: " + std::string(e.what()));
        return false;
    }
}