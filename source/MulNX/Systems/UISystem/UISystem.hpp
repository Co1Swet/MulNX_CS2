#pragma once
#include "UIContext/UIContext.hpp"
#include <MulNX/Core/Module/Module.hpp>
#include <MulNXExtensions/WinExt/WIN32Msg/WIN32Msg.hpp>
#include <MulNXThirdParty/queue/concurrentqueue.h>

namespace MulNX {
    class UISystem final :public MulNX::Module<UISystem> {
    private:
        MulNX::UIContext UIContext{};
        bool UISystemRunning = false;
        
        std::string strImguiIniPath;

        void LoadFont();
        void LoadStyle();
        void SaveStyle();

        bool Menu(MulNX::UINode* node);
    public:
        std::atomic<bool>WantCaptureMouse{ false };
        std::atomic<bool>WantTextInput{ false };
        moodycamel::ConcurrentQueue<MulNX::Win32::Msg4>winMsgs{};
        std::function<bool(void)>FrameBefore = nullptr;
        std::function<void(void)>FrameBehind = nullptr;

        bool Init()override;
        void ProcessMsg(MulNX::Message& Msg)override;
        void HandleUpdate();
        int Render();

        MulNX::UIContext* GetUIContext() { return &this->UIContext; }
    };
}