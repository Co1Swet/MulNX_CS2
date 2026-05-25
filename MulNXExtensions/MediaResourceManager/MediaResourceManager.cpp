#include "MediaResourceManager.hpp"
#include <avcpp/av.h>
#include <avcpp/codec.h>
#include <avcpp/averror.h>

bool MediaResourceManager::Init() {
    // 初始化 FFmpeg 所有组件
    av::init();
    av::set_logging_level(AV_LOG_WARNING);  // 减少 FFmpeg 自身输出

    this->ISys().LogSucc("FFmpeg 与 AvCpp 初始化成功！");

    return true;
}