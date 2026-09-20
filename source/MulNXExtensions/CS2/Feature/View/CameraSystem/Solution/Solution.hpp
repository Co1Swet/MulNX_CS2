#pragma once
#include <MulNX/MulNX.hpp>
#include <CameraSystem/FreeCameraPath/FreeCameraPath.hpp>
#include <CameraSystem/CamPlayScheduler/CamPlayRequest.hpp>

class ElementWithOffset {
public:
    //元素
    std::string campathName;
    //这个Offset决定了元素的播放时间头
    float Offset = 0;
};

//解决方案，包含ElementWithOffset，用于调用call方法
class Solution final {
    std::string name{};
    std::vector<ElementWithOffset> elements;

    float startTime{};
    float endTime{};
    float totalDurationTime{};
    // 播放模式
    PlaybackMode playmode = PlaybackMode::Orchestration;

    // 按键检测包
    std::atomic<MulNX::KeyCheckPack> KCPack{};
    // 脏标记，需要重新保存
    bool dirty = false;
public:
    Solution(const std::string& name) :
        name(name) {
        
    }

    // 添加元素
    bool AddElement(const std::string& name, const float offset);
    // 移除指定位置的元素
    bool RemoveElementAt(const size_t Index);
    inline const std::vector<ElementWithOffset>& GetVec()const { return this->elements; }

    std::pair<bool, std::string> Save(const std::filesystem::path& folderPath)const;
    std::pair<bool, std::string> Load(YAML::Node& root);

    //清空数据
    void Clear();
    //重设名字
    void ResetName(std::string_view NewName);
    //获取名字
    const std::string& GetName()const;
    //展示信息
    std::string GetMsg();

    inline const std::atomic<MulNX::KeyCheckPack>& GetKeyCheckPack()const { return this->KCPack; }
    void SetKeyCheckPack(const MulNX::KeyCheckPack& KCPack);

    std::vector<CamPlayRequest> GetRequests();
};