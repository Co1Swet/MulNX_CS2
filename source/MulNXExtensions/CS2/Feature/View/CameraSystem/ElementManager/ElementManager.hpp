#pragma once
#include <CameraSystem/CameraSystem.hpp>
#include <CameraSystem/FreeCameraPath/FreeCameraPath.hpp>
#include <MulNXUtils/NewestBuffer.hpp>
#include "ElementConfig.hpp"

// 元素管理器，用于管理元素
class ElementManager final : public CamSysModule {
    CameraDrawer* CamDrawer = nullptr;
    SolutionManager* SManager = nullptr;
    ProjectManager* PManager = nullptr;
    MulNX::IPCer* pIPCer = nullptr;

    // 当前操作的元素指针
    std::atomic<std::shared_ptr<FreeCameraPath>> CurrentElement = nullptr;

    // 展示单个元素信息在一行上
    void Element_ShowInLine(const std::shared_ptr<const FreeCameraPath> element)const;

    void DebugUI(const FreeCameraPath* campath)const;
    void UINodeFunc()const;
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;

    FreeCameraPath* Element_Create(const std::string& name);
    // 保存所有元素到磁盘文件
    bool Element_SaveAll();
    // 从磁盘文件加载元素的预处理函数，内部会创建对应类型的元素，并调用具体加载函数加载信息
    bool Element_Load(const std::filesystem::path& FullPath);
    // 删除函数元素，返回true表示名称现在可用
    bool Element_Delete(const std::string Name);
    // 清空所有元素
    bool Element_ClearAll();
public:
    ElementConfig Config{};
    // 使用智能指针存储多态对象，以存储不同类型的元素
    std::unordered_map<std::string, std::shared_ptr<FreeCameraPath>> elements;
    std::shared_ptr<FreeCameraPath> FindCampath(const std::string& name);
    bool HandleUpdate(CameraSystemIO* IO);

    bool MenuElement()const;
};