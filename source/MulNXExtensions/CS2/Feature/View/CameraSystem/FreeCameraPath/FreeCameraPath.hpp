#pragma once
#include <MulNX/MulNX.hpp>
#include <CameraSystem/CameraSystemIO/CameraSystemIO.hpp>
#include <yaml-cpp/yaml.h>
#include <string>
#include <filesystem>

class CameraDrawer;
class ElementManager;

// 自由摄像机轨道，继承自Element
class FreeCameraPath final {
public:
    // 元素名称
    std::string Name{};

    // 开始时间（绝对）
    float StartTime{};
    // 结束时间（绝对）
    float EndTime{};
    // 持续时长
    float DurationTime{};

    // 需要被删除
    bool NeedBeDelete = false;
    // 脏标记，需要重新保存
    bool Dirty = false;
    // 是否绘制（默认不绘制）
    std::atomic<bool> draw = false;

    std::vector<MulNX::Math::CameraKeyframe> CameraKeyframes{};
    // 构造函数
    explicit FreeCameraPath(const std::string& name) : 
        Name(name) {}
    
    
    // 刷新状态
    void Refresh();
    
    // 获取详细信息
    std::string GetPrivateMsg()const;
    
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
    bool DrawBase(CameraDrawer* CamDrawer, const float* Matrix, const float WinWidth, const float WinHeight)const;
    void DebugUI(ElementManager* EManager);

    // 获取基本信息
    std::string GetBaseInfo()const;
    std::string GetMsg()const;
    // 获取名字
    std::string GetName()const;
    // 重设名字
    void ResetName(const std::string& NewName);

    float GetStartTime()const;
    float GetEndTime()const;
    float GetDurationTime()const;

    //（Mode:0为默认，1自动减去头时间）
    bool CalculateFrame(CameraSystemIO* IO)const;
};