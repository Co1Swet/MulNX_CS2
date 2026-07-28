#include "HookRecordCmd.hpp"
#include <Intro/HookConsole/HookConsole.hpp>

bool HookRecordCmd::Init() {
    this->SubscribeSync("Hook/RegisterConCommand", [this](MulNX::Message& msg) {
        auto&& [pCmd] = msg.Access<CCmd*>();

        this->LogInfo(pCmd->m_pszName);
        if (std::string(pCmd->m_pszName).find("startmovie") != std::string::npos) {
            pCmd->m_nFlags = 8;
        }

        if (std::string(pCmd->m_pszName).find("endmovie") != std::string::npos) {
            pCmd->m_nFlags = 8;
        }

        });

    return true;
}