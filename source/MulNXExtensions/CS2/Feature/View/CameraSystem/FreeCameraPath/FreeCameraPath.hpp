#pragma once
#include <MulNX/MulNX.hpp>
#include <CameraSystem/CameraSystemIO/CameraSystemIO.hpp>
#include <yaml-cpp/yaml.h>
#include <string>
#include <filesystem>

class CameraDrawer;

class FreeCameraPath final {
    std::string name{};
    bool dirty = false;
    // 开始时间（绝对）
    float startTime{};
    // 结束时间（绝对）
    float endTime{};
    float durationTime{};
public:
    inline bool IsDirty()const { return this->dirty; }

    inline float GetStartTime()const { return this->startTime; }
    inline float GetEndTime()const { return this->endTime; }
    inline float GetDurationTime()const { return this->durationTime; }

    // 是否绘制（默认不绘制）
    std::atomic<bool> draw = false;

    std::vector<MulNX::Math::CameraKeyframe> CameraKeyframes{};
    // 构造函数
    explicit FreeCameraPath(const std::string& name) :
        name(name) {}

    // 刷新状态
    void Refresh();

    // 获取基本信息
    std::string GetBaseInfo()const;
    std::string GetMsg()const;

    // 增加关键帧
    void AddKeyframe(const MulNX::Math::CameraKeyframe& KeyFrame);
    // 归一化关键帧时间
    void TimeNormalize();
    // 清空所有关键帧
    void Clear();

    const MulNX::Math::CameraKeyframe& GetKeyFrame(const size_t& index)const;// 获取特定关键帧    

    // 磁盘IO
    std::pair<bool, std::string> Save(const std::filesystem::path& folderPath);
    std::pair<bool, std::string> SaveImpl(YAML::Node& root);
    std::pair<bool, std::string> Load(YAML::Node& root);

    // 绘制
    bool Draw(CameraDrawer* CamDrawer, const float* Matrix, const float WinWidth, const float WinHeight)const;

    
    // 获取名字
    const std::string& GetName()const;
    // 重设名字
    void ResetName(const std::string& NewName);

    enum class CalResult :int8_t {
        Before,
        In,
        Back,
        NoFrame
    };

    CalResult CalculateFrame(CameraSystemIO* IO)const;
};