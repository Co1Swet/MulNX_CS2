#pragma once

#include <MulNX/MulNX.hpp>
#include <MulNX/Base/NewestBuffer/NewestBuffer.hpp>

class DemoHelper final :public MulNX::ModuleBase {
private:
    std::vector<float>Marks{};
    std::atomic<MulNX::any_unique_ptr*>* ppUpdateData = nullptr;
    MulNXHandle hUINode{};
    std::vector<float> TimeMarks{};
public:

    bool Init()override;

    void Main();
    void ProcessMsg(MulNX::Message& msg)override;
    bool UINodeFunc(MulNX::UINode* node);
    //void HandleUICommand(MulNXMessage* Msg);

    bool MarkTime();
    bool JumpMark(size_t Index);
};