#pragma once
#include <MulNX/MulNX.hpp>
#include <CameraSystem/CameraSystemIO/CameraSystemIO.hpp>
#include <yaml-cpp/yaml.h>
#include <string>
#include <filesystem>

//元素的类型在MulNX::Base的CameraSystemIO中定义

class CameraDrawer;

std::string ElementType_EnumToString(const ElementType Type);
ElementType ElementType_StringToEnum(const std::string& typeStr);

//元素基类，内含call方法修改target
// class ElementBase {
//     //对于元素基类，持续时长和名称是所有元素都需要的属性，我们还需要一个纯虚函数Call用于多态调用，每个元素可以有自己的数据和实现，尤其针对Call函数
//     //元素的创建、获取、删除由ElementManager统一管理
//     //Call函数的通用调用是CameraSystem的实现的核心，因为CameraSystem只需要调用Call函数而不需要知道具体元素的类型，各种Call会在各种不同的地方调用
//     //元素的具体功能由各个元素类自己实现
// protected:

//     //基本数据存储

//     //元素类型
//     //模板类型
//     static constexpr ElementType TemplateType = ElementType::ElementBase;
    
// public:
//     //构造函数
//     ElementBase() = default;
//     explicit ElementBase(const std::string& name) :
//         Name(name) {
//         this->Type = ElementType::ElementBase;
//     }


// public:
    
// };

class ElementManager;

// 自由摄像机轨道，继承自Element
class FreeCameraPath final {
    // 友元声明
    friend ElementManager;
public:
    // 模板类型
    static constexpr ElementType TemplateType = ElementType::FreeCameraPath;
    // 额外数据存储
    std::vector<MulNX::Math::CameraKeyframe> CameraKeyframes{};
    // 构造函数
    explicit FreeCameraPath(const std::string& name) : 
        Name(name) {
		this->Type = ElementType::FreeCameraPath;
        this->Drawable = true;
    }
public:
    //实际类型
    ElementType Type;
    //元素名称
    std::string Name{};

    //开始时间（绝对）
    float StartTime{};
    //结束时间（绝对）
    float EndTime{};
    //持续时长
    float DurationTime{};

    //需要被删除
    bool NeedBeDelete = false;
    //脏标记，需要重新保存
    bool Dirty = false;
    //能否绘制（默认不能）
    bool Drawable = false;
    //是否绘制（默认不绘制）
    std::atomic<bool> draw = false;
    
    // 虚函数类
    // 刷新状态（虚）
    void Refresh();
    // 调用函数（虚）（Mode:0为默认，1自动减去头时间）
    bool Call(CameraSystemIO* IO)const;
    // 绘制函数（虚），各个元素按需实现
    bool Draw(CameraDrawer* CamDrawer, const float* Matrix, const float WinWidth, const float WinHeight)const;
    // 获取详细信息（虚）
    std::string GetPrivateMsg()const;
    
    // 非虚函数类
    // 增加关键帧
	void AddKeyframe(const MulNX::Math::CameraKeyframe& KeyFrame);
    // 归一化关键帧时间
    void TimeNormalize();
    // 清空所有关键帧
    void Clear();
    
    size_t GetKeyFrameCount()const;// 获取关键帧数量
    const MulNX::Math::CameraKeyframe& GetKeyFrame(const size_t& index)const;// 获取特定关键帧    
    const std::vector<MulNX::Math::CameraKeyframe>& GetAllKeyFrames()const;// 获取所有关键帧

    std::pair<bool, std::string> SaveImpl(YAML::Node& root);
    std::pair<bool, std::string> Load(YAML::Node& root);

    void DebugUI(ElementManager* EManager);

    //ID相关

    // 获取枚举类型
    ElementType TypeGet_Enum()const;
    // 获取字符串类型
    std::string TypeGet_String()const;


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


    bool CalculateFrame(CameraSystemIO* IO)const;
protected:

public:

    // 通用Call，用于时间变换，基类型绑定等
    // bool BaseCall(float& OutputTime, CameraSystemIO* IO)const;

    // 绘制函数入口
    bool DrawBase(CameraDrawer* CamDrawer, const float* Matrix, const float WinWidth, const float WinHeight)const;

    std::pair<bool, std::string> Save(const std::filesystem::path& folderPath);
};

namespace Elements {
    const std::vector<std::string> strTypes = { "自由摄像机轨道" };
}