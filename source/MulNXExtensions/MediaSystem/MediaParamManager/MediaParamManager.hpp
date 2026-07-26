#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>
#include <MulNXExtensions/MediaSystem/MediaParamManager/RecordParams.hpp>

class MediaParamManager :public MediaModuleBase{
    RecordParams params;
    bool Init()override;
public:
    // 当前生效的录制参数（可被 UI/消息修改）
    RecordParams& Params() { return this->params; }
    const RecordParams& Params() const { return this->params; }
};
