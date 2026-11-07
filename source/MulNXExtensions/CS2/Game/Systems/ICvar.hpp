#pragma once
#include "IAppSystem.hpp"
#include <Game/Consoles.hpp>
#include <Game/CSUtils/CUtlString.hpp>

// .data:00000001803A2660                                                 ; public class CCvar /* mdisp:0 */ :
// .data:00000001803A2660;   public class CTier0AppSystem<class ICvar, 0> /* mdisp:0 */ :
// .data : 00000001803A2660;     public class CBaseAppSystem<class ICvar> /* mdisp:0 */ :
// .data : 00000001803A2660;       public class ICvar /* mdisp:0 */ :
// .data : 00000001803A2660;         public class IAppSystem /* mdisp:0 */
// .data :00000001803A2660; class CCvar `RTTI Type Descriptor'
// .data:00000001803A2660 20 C4 2F 80 01 00 00 00 ? ? _R0 ? AVCCvar@@@8 dq offset ? ? _7type_info@@6B@
// .data:00000001803A2660; DATA XREF : .rdata : 0000000180341EC4↑o
// .data : 00000001803A2660;.rdata:CCvar::`RTTI Base Class Descriptor at(0, -1, 0, 64)'↑o
// .data:00000001803A2660; reference to RTTI's vftable
// .data:00000001803A2668 00 00 00 00 00 00 00 00                                         dq 0; internal runtime reference
// .data:00000001803A2670 2E 3F 41 56 43 43 76 61 72 40 40 00             aAvccvar        db '.?AVCCvar@@', 0; type descriptor name

// 这里省略了很多，但是也只需要虚函数

namespace CS2 {
    class CPlayerSlot {
    public:
        int data;
    };

    class CSplitScreenSlot {
    public:
        int data;
    };
}

namespace CS2::Systems {
    class CCvar :public IAppSystem {
    public:
        class HConItem {
        public:
            size_t index = 0x000000000000FFFF;
            bool IsValid()const { return this->index != 0x000000000000FFFF && this->index != 0x00000000FFFFFFFF; }
        };
        
        class HConVar :public HConItem {};
        class HConCommand :public HConItem {};

        class CCommandContext {
        public:
            enum class CommandTarget_t :int {
                CT_NO_TARGET = -1,
                CT_FIRST_SPLITSCREEN_CLIENT = 0,
                CT_LAST_SPLITSCREEN_CLIENT = 3,
            };
            CommandTarget_t eTarget;
            CPlayerSlot nPlayerSlot;
        };

        virtual HConVar* FindConVar(HConVar* pHConVar, const char* name, bool bDiallowDeveloper = true) = 0; // 011
        virtual HConVar* FindFirstConVar(HConVar* pHConVar) = 0; // 012
        virtual HConVar* FindNextConVar(HConVar* pHConVar, HConVar hPrev) = 0; // 013
        virtual void CallChangeCallback(HConVar cvar, const CS2::CSplitScreenSlot nSlot,
            const CS2::Console::ConVar::Value_t* pNewValue, const CS2::Console::ConVar::Value_t* pOldValue,
            void* unk = nullptr) = 0; // 014
        virtual void unk015(void) = 0;
        virtual void unk016(void) = 0;
        virtual HConCommand* FindConCommand(HConCommand* pHConCommand, const char* name, bool bDiallowDeveloper = true) = 0; // 017
        virtual HConCommand* FindFirstConCommand(HConCommand* pHandle) = 0; // 018
        virtual HConCommand* FindNextConCommand(HConCommand* pHPrev, HConCommand hPrev) = 0; // 019
        virtual void DispatchConCommand(HConCommand cmd, const CCommandContext& ctx, const CS2::Console::CCommand& args) = 0; // 020
        virtual void unk021(void) = 0;
        virtual void unk022(void) = 0;
        virtual void unk023(void) = 0;
        virtual void unk024(void) = 0;
        virtual void unk025(void) = 0;
        virtual void unk026(void) = 0;
        virtual void unk027(void) = 0;
        virtual void unk028(void) = 0;
        virtual void unk029(void) = 0;
        virtual void unk030(void) = 0;
        virtual void unk031(void) = 0;
        virtual void unk032(void) = 0;
        virtual void unk033(void) = 0;
        virtual void unk034(void) = 0;
        virtual void unk035(void) = 0;
        virtual void unk036(void) = 0;
        virtual void unk037(void) = 0;
        virtual void unk038(void) = 0;
        virtual void unk039(void) = 0;
        virtual void unk040(void) = 0;
        virtual CS2::Console::ConVar* GetConVar(HConVar hConVar) = 0; // 041

        // 这里注意，游戏自己关于RegisterConCommand的调用
        // 调用后如果再get，那个callback就会被重定向到一个包装（至少原始指针模式是这样的）
        // 如果hook这个注册，我们就可以拿到原始回调指针，实际上之前是这么做的，现在信息更清晰了

        virtual HConCommand* RegisterConCommand(HConCommand* pIt, CS2::Console::ConCommand* pCmd, int64_t nAdditionalFlags = 0) = 0; // 042
        virtual void UnregisterConCommand(size_t i) = 0; // 043
        virtual CS2::Console::ConCommand* GetConCommand(size_t i) = 0; // 044
    };
}