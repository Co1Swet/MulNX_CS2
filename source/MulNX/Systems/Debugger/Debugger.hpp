#pragma once
#include <MulNX/Common/Message.hpp>
#include <MulNX/Core/Module/Module.hpp>
#include <deque>

namespace MulNX {
    class Debugger final :public MulNX::Module<Debugger> {
        std::string kInfo{};
        std::string kSucc{};
        std::string kWarning{};
        std::string kError{};
        std::deque<std::string> debugMsg{};

        int maxMsgCount = 1000;
        bool showWhenError = true;
        bool autoScroll = true;
		bool needAutoScroll = false;

        void Main();
        void Window();
        void DeMe();
        void ResetMaxMsgCount(const int Max);

		bool Init()override;
		void ProcessMsg(MulNX::Message& Msg)override;        
	};
}