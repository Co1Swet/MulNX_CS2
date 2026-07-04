#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>
#include <MulNXExtensions/MediaSystem/MediaParamManager/RecordParams.hpp>

class MediaParamManager :public MediaModuleBase{
    EncoderCaps caps;
    RecordParams params;
public:
    bool Init()override;

    // 启动时探测到的编码器能力（只读）
    const EncoderCaps& Caps() const { return this->caps; }

    // 当前生效的录制参数（可被 UI/消息修改）
    RecordParams& Params() { return this->params; }
    const RecordParams& Params() const { return this->params; }
};
