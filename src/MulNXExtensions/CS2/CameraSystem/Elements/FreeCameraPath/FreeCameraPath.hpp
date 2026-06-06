#pragma once

#include <MulNXExtensions/CS2/CameraSystem/Elements/ElementBase.hpp>

class ElementManager;

// 自由摄像机轨道，继承自Element
class FreeCameraPath final :public ElementBase {
    // 友元声明
    friend ElementManager;
public:
    // 模板类型
    static constexpr ElementType TemplateType = ElementType::FreeCameraPath;
    // 额外数据存储
    std::vector<MulNX::Math::CameraKeyframe> CameraKeyframes{};
    // 构造函数
    explicit FreeCameraPath(const std::string& name) : 
        ElementBase(name) {
		this->Type = ElementType::FreeCameraPath;
        this->Drawable = true;
    }
public:
    // 虚函数类
    // 刷新状态（虚）
    void Refresh()override;
    // 调用函数（虚）（Mode:0为默认，1自动减去头时间）
    bool Call(CameraSystemIO* IO)const override;
    // 绘制函数（虚），各个元素按需实现
    bool Draw(CameraDrawer* CamDrawer, const float* Matrix, const float WinWidth, const float WinHeight)const override;
    // 获取详细信息（虚）
    std::string GetPrivateMsg()const override;
    
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

    std::pair<bool, std::string> SaveImpl(YAML::Node& root)override;
    std::pair<bool, std::string> Load(YAML::Node& root)override;

    void DebugUI(ElementManager* EManager)override;
};