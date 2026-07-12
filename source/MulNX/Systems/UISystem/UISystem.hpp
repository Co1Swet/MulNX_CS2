#pragma once
#include <MulNX/Systems/UISystem/UICoordinator/UICoordinator.hpp>
#include <MulNXUtils/WinExt/WIN32Msg/WIN32Msg.hpp>
#include <MulNXThirdParty/queue/concurrentqueue.h>

namespace MulNX {
    class UISystem final :public MulNX::Module<UISystem> {
    private:
        UICoordinator* pCoordinator = nullptr;        
        std::string strImguiIniPath;

        void LoadFont();
        void LoadStyle();
        void SaveStyle();

        bool Menu();
        bool Init()override;
        void ProcessMsg(MulNX::Message& Msg)override;
    public:
        std::atomic<bool>WantCaptureMouse{ false };
        std::atomic<bool>WantTextInput{ false };
        moodycamel::ConcurrentQueue<MulNX::Win32::Msg4>winMsgs{};
        std::function<bool(void)>FrameBefore = nullptr;
        std::function<void(void)>FrameBehind = nullptr;

        void HandleUpdate();
        int Render();
    };
}