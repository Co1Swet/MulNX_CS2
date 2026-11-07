#pragma once
#include <Game/CSUtils/CUtlString.hpp>
#include <cstdint>
#include <functional>

namespace CS2::Console {
    // 控制台指令原始文本
#pragma pack(push, 8) // 强制 8 字节对齐
    class CCommand {
    public:
        // 依赖声明
        struct characterset_t;
        // Init是引擎关于CCommand的构造函数，在这里给它起一个名字
        using Init_t = CCommand * (*)(CCommand* pCommand);
        inline static Init_t pInit = nullptr;
        // 这是来自引擎的分词器，这里注意只用nullptr，不然还要逆向characterset_t，传nullptr引擎会自己找默认的
        using Tokenize_t = bool(*)(CCommand* pCommand, const char* string, const characterset_t* characterset);
        inline static Tokenize_t pTokenize = nullptr;

        // 数据声明
        int unkSize;              // 0x00 (4字节)
        // 0x04 处编译器自动填充 4 字节对齐
        char* pad;                // 0x08 (8字节)
        char* pRawString;         // 0x10 (8字节)
        // 剩下的全部用 0x650 字节填满，直到 0x668
        std::byte unknown[0x650];

        // 方法声明
        CCommand() {
            // 将整个 1640 字节的内存区域全部填 0，毕竟我们不会构造，填成零转交引擎函数去构造
            std::memset(this, 0, sizeof(*this));
            pInit(this);
        }
        inline bool Tokenize(const char* string) {
            return this->pTokenize(this, string, nullptr); // 这里一定传nullptr
        }
    };
#pragma pack(pop)
    // 验证大小：0x18 (头部24字节) + 0x650 (填充) = 0x668 (1640字节)
    static_assert(sizeof(CCommand) == 0x668, "CCommand size mismatch!");

    // 控制台变量标记位
    enum EConVarFlag : int {
        FCVAR_CLIENTDLL = (1 << 3),
        FCVAR_HIDDEN = (1 << 4)
    };

    class ConVar {
    public:
        union Value_t {
            bool m_bValue;
            short m_i16Value;
            uint16_t m_u16Value;
            int	m_i32Value;
            uint32_t m_u32Value;
            int64_t	m_i64Value;
            uint64_t m_u64Value;
            float m_flValue;
            double m_dbValue;
            CS2::CUtlString	m_szValue;
        };

        enum class EConVarType : int16_t {
            Invalid = -1,
            Bool,
            Int16,
            UInt16,
            Int32,
            UInt32,
            Int64,
            UInt64,
            Float32,
            Float64,
            String,
            Color,
            Vector2,
            Vector3,
            Vector4,
            Qangle,
            VectorWS,
            MAX
        };

        const char* szName; // 0x0000
        Value_t* m_defaultValue;
        Value_t* m_minValue;
        Value_t* m_maxValue;
        const char* m_pszHelpString;
        EConVarType m_eVarType;
        short m_Version;
        unsigned int m_iTimesChanged;
        int64_t m_nFlags;
        unsigned int m_iCallbackIndex;
        unsigned int m_iFilterCBIndex;
        int m_GameInfoFlags;
        int m_UserInfoByteIndex;
        void* unk0x48;
        void* unk0x50;
        Value_t m_Value;

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

        ConVar() = delete;
        ~ConVar() = delete;
    };

    class ConCommand {
    public:
        class IConCommandCallback {
            virtual void ConCommandCallback(void* rdx, CCommand* pArgs) = 0;
        };

        const char* m_pszName;
        const char* m_pszHelpString;
        int64_t m_nFlags;
        union {
            void* pConCmdCallback;
            IConCommandCallback* IConCmdCallback;
        };
        // 这些标志位让引擎采用虚函数callback而不是原始函数指针
        uint64_t unk32 = 0x0101;
        uint64_t unk40 = 0;
        uint64_t unk48 = 0x01;
        uint64_t m_NextCommand = 0xFFFFFFFF;

        ConCommand(const char* pszName, const char* pszHelpString, int64_t nFlags, IConCommandCallback* pCommandCallback) {
            m_pszName = pszName;
            m_pszHelpString = pszHelpString;
            m_nFlags = nFlags;
            IConCmdCallback = pCommandCallback;
        }
    };
}

// 将自由函数转为 ICommandCallback 的适配器
class MulNXCS2CmdCallback : public CS2::Console::ConCommand::IConCommandCallback {
    std::move_only_function<void(CS2::Console::CCommand*)> callback;
    void ConCommandCallback(void*, CS2::Console::CCommand* args)override {
        this->callback(args);
    }
public:
    MulNXCS2CmdCallback(std::move_only_function<void(CS2::Console::CCommand*)> f) : callback(std::move(f)) {}
};