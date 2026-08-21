#pragma once
#include <Intro/CSModuleBase.hpp>

class DemoHelper final :public CSModuleBase {
    std::vector<float>Marks{};
    std::atomic<MulNX::any_unique_ptr*>* ppUpdateData = nullptr;
    MulNXHandle hUINode{};
    std::vector<float> TimeMarks{};
    bool Init()override;
    void Main();
    void ProcessMsg(MulNX::Message& msg)override;
    void Window();

    bool MarkTime();
    bool JumpMark(size_t Index);
};