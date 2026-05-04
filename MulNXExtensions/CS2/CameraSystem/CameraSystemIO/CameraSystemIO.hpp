#pragma once

#include <MulNX/MulNX.hpp>

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

// 基元素类型
enum class ElementType :int {
	None = -1,// 错误类型
	ElementBase = 0,// 元素基类
	FreeCameraPath = 1,// 自由摄像机轨道
};

// 待添加的附加元素类型，如景深滤镜

class CameraSystemIO {
public:
	MulNX::Math::Frame Frame;// 当前帧数据
	float FrameGameTime;// 用于参考的绝对游戏时间
	float SolutionTime;// 解决方案时间轴时间
	float ElementTime;// 元素时间轴时间，这个时间是由解决方案或预览负责提供的，基于元素的绝对时间轴
	bool isPlaying;// 当前是否在播放，解决方案会修改这个值
};