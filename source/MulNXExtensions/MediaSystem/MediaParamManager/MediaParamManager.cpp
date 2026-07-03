#include "MediaParamManager.hpp"

bool MediaParamManager::Init() {
    this->caps = DetectEncoderCaps();

    this->LogSucc(std::format("编码器能力探测完成：硬件={} 软件={} D3D11VA={}",
        this->caps.hwEncoders.size(),
        this->caps.swEncoders.size(),
        this->caps.d3d11vaAvailable ? "是" : "否"));

    if (!this->caps.hwEncoders.empty()) {
        std::string joined;
        for (const auto& n : this->caps.hwEncoders) joined += n + " ";
        this->LogInfo(std::format("可用硬件编码器: {}", joined));
    } else {
        this->LogWarning("未检测到硬件编码器，将使用软件编码");
    }
    if (!this->caps.swEncoders.empty()) {
        std::string joined;
        for (const auto& n : this->caps.swEncoders) joined += n + " ";
        this->LogInfo(std::format("可用软件编码器: {}", joined));
    }

    return true;
}
