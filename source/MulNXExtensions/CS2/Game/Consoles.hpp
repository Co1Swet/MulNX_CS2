#pragma once
#include <cstdint>
#include <functional>

namespace CS2::Console {
    // 控制台指令原始文本
    class CCommandLine {
    public:
        // 不要自行构建
        CCommandLine() = delete;
        int unkSize;
        char* pad;
        char* pRawString;
    };

    // 控制台变量标记位
    enum EConVarFlag : int {
        FCVAR_CLIENTDLL = (1 << 3),
        FCVAR_HIDDEN = (1 << 4)
    };
    
    class C_ConVar {
    public:
        const char* szName; // 0x0000
        //CVValue_t* m_pDefaultValue; // 0x0008
        //char PAD[8]; // 0x0010
        //const char* szDescription; // 0x0020
        //uint32_t nType; // 0x28
        //uint32_t nRegistered; // 0x2C
        //uint32_t nFlags; // 0x30
        //char PAD2[0x1C]; // 0x34
        //CVValue_t value; // 0x50

        template<typename T>
        T* GetPtr() {
            T* pValue = nullptr;
            pValue = reinterpret_cast<T*>((uintptr_t)(this) + 0x58);
            return pValue;
        }

        void Hide() {
            *reinterpret_cast<uint32_t*>((uintptr_t)(this) + 0x30) |= FCVAR_HIDDEN;
        }
        void Unhide() {
            *reinterpret_cast<uint32_t*>((uintptr_t)(this) + 0x30) &= ~FCVAR_HIDDEN;
        }
        bool IsHidden()const {
            return *reinterpret_cast<uint32_t*>((uintptr_t)(this) + 0x30) & FCVAR_HIDDEN;
        }
    };

    // size: 8*8 Bytes
    class ConConmand {
    public:
        class IConConmandCallback {
            virtual void ConConmandCallback(void* rdx, CCommandLine* pArgs) = 0;
        };
        
        const char* m_pszName;
        const char* m_pszHelpString;
        int64_t m_nFlags;
        union {
            void* pConCmdCallback;
            IConConmandCallback* IConCmdCallback;
        };
        
        // 这些标志位让引擎采用虚函数callback而不是原始函数指针
        uint64_t unk32 = 0x0101;
        uint64_t unk40 = 0;
        uint64_t unk48 = 0x01;
        uint64_t m_NextCommand = 0xFFFFFFFF;

        ConConmand(const char* pszName, const char* pszHelpString, int64_t nFlags, IConConmandCallback* pCommandCallback) {
            m_pszName = pszName;
            m_pszHelpString = pszHelpString;
            m_nFlags = nFlags;
            IConCmdCallback = pCommandCallback;
        }
    };
}

// 将自由函数转为 ICommandCallback 的适配器
class MulNXCS2CmdCallback : public CS2::Console::ConConmand::IConConmandCallback {
    std::move_only_function<void(CS2::Console::CCommandLine*)> callback;
    void ConConmandCallback(void*, CS2::Console::CCommandLine* args)override {
        this->callback(args);
    }
public:
    MulNXCS2CmdCallback(std::move_only_function<void(CS2::Console::CCommandLine*)> f) : callback(std::move(f)) {}
};