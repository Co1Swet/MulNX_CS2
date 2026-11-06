#pragma once
#include <MulNX/MulNX.hpp>
#include <CS2Math/Math.hpp>

// 播放模式：激活模式与编排模式
enum class PlaybackMode {
    Activation,     // 激活模式（元素按照游戏原初时间线播放）
    Orchestration   // 编排模式（元素按照被分配的相对时间进行播放）
};
inline std::string PlaybackModeToString(PlaybackMode mode) {
    switch (mode) {
    case PlaybackMode::Activation:return"激活模式";
    case PlaybackMode::Orchestration:return"编排模式";
    }
    return "错误";
}

class CameraSystemIO {
public:
	MulNX::Math::Frame Frame;// 当前帧数据
	double FrameGameTime{};// 用于参考的绝对游戏时间
	float SolutionTime;// 解决方案时间轴时间
	double ElementTime{};// Camera-only high precision; keyframe storage stays float.
	bool PreciseTimeSampling = false;
	bool isPlaying;// 当前是否在播放，解决方案会修改这个值
};
