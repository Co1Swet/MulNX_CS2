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

        MulReflInt(maxMsgCount, 1000);
        MulReflBool(showWhenError, true);
        MulReflBool(autoScroll, true);
        
        std::atomic<bool> needAutoScroll = false;

        void Window();
        void DeMe();

        void AddMsg(std::string&& fmtted);
        void ResetMaxMsgCount(const int Max);

		bool Init()override;
		void ProcessMsg(MulNX::Message& Msg)override;        
	};
}