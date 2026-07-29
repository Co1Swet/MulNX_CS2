#include "HookRecordCmd.hpp"
#include <Intro/HookConsole/HookConsole.hpp>

bool HookRecordCmd::Init() {
    this->SubscribeSync("Hook/RegisterConCommand", [this](MulNX::Message& msg) {
        auto&& [pCmd] = msg.Access<CCmd*>();

        // this->LogInfo(pCmd->m_pszName);

        if (strcmp(pCmd->m_pszName, "startmovie") == 0) {
            pCmd->m_nFlags = 8;
        }
        if (strcmp(pCmd->m_pszName, "endmovie") == 0) {
            pCmd->m_nFlags = 8;
        }

        });

    return true;
}