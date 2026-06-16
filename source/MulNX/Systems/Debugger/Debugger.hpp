#pragma once
#include <MulNX/Common/Message.hpp>
#include <MulNX/Core/Module/Module.hpp>
#include <deque>

namespace MulNX {
    class Debugger final :public MulNX::Module<Debugger> {
    private:
        MulNX::Logger* pLogger = nullptr;
        std::string kInfo{};
        std::string kSucc{};
        std::string kWarning{};
        std::string kError{};
        std::deque<std::string> DebugMsg{};

        int MaxMsgCount = 1000;
        bool ShowWhenError = true;
        bool AutoScroll = true;
		bool NeedAutoScroll = false;

        void Main();
        bool Window(MulNX::UINode* node);
        void DeMe(MulNX::UINode* node);
        void ResetMaxMsgCount(const int Max);

		bool Init()override;
		void ProcessMsg(MulNX::Message& Msg)override;        
	};
}