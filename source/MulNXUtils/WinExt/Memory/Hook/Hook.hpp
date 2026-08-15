#pragma once
#include <MulNXUtils/WinExt/Memory/AddrTool/AddrTool.hpp>
#include <MulNX/Common/Common.hpp>
#include <MulNXUtils/WinExt/Memory/Assembler/Assembler.hpp>
#include <functional>
#include <expected>

namespace MulNX {
    class Hook final {
    public:
        enum class Result :uint8_t {
            Attached,
            AttachSuccess,
            AttachError,

            Detached,
            DetachSuccess,
            DetachError
        };

        enum class Then {
            Continue,
            SkipAllAndContinue,
            JmpUserSettedTarget,
            Return
        };
    private:
        bool attached = false;
        std::function<Then(Hook*, RegContext*)> callback;
        std::atomic<size_t> threadNumInAsm = 0;

        void* pAsmDispatcher = nullptr;
        MulNX::Memory::Asm::Code dispatcherAsmCode{};

        Addr::TargetInfo hookTargetInfo{};
        uint8_t* hookTarget = nullptr;
        size_t overrideSize = 0;
        MulNX::Memory::Asm::Code hookTargetRawCode{};
        MulNX::Memory::Asm::Code hookTargetFixedCode{};
        MulNX::Memory::Asm::Code jumperAsmCode{};

        struct JmpTarget {
            uintptr_t Return = 0;
            uintptr_t Continue = 0;
            uintptr_t SkipAllAndContinue = 0;
            uintptr_t UserSettedTarget = 0;
        };
        JmpTarget jmpTarget{};


        size_t frameSize = 0;
        uintptr_t pCallOrigin = 0;

        uintptr_t Dispatch(RegContext* ctx);
        void CopyStack(size_t copySize, RegContext* ctx, uintptr_t pCurStack);


        std::optional<std::string> BindTarget(uint8_t* target, int len);
        std::optional<std::string> CreateAFix();
        std::optional<std::string> CreateStart(bool extraStackAdjust, bool callRawFisrt);
    public:
        ~Hook();
        
        static std::expected<std::unique_ptr<Hook>, std::string> Create(
            uint8_t* target, std::function<MulNX::Hook::Then(Hook*, RegContext*)>&& callback,
            bool extraStackAdjust = false, bool callRawFisrt = false,
            uintptr_t userJmpTarget = 0, int len = 0);

        Result Attach();
        Result Detach();

        void ResetCallback(std::function<MulNX::Hook::Then(Hook*, RegContext*)>&& callback);
        uintptr_t pMaybeRawFunc = 0;
        uint64_t CallMaybeOrigin(size_t copyStackParamNum, RegContext* ctx);
        void* GetRawStackAddr(RegContext* ctx);
        template<MulNX::PodSizeIn<0, 8> T>
        T* GetStackParam(RegContext* ctx, size_t num) {
            auto stack = reinterpret_cast<uintptr_t>(this->GetRawStackAddr(ctx));
            T* param = reinterpret_cast<T*>(stack + 0x8 + num * 0x8);
            return param;
        }
        template<typename F, typename... Args>
        auto CallMaybeAs(Args... args) {
            return reinterpret_cast<F>(this->pMaybeRawFunc)(std::forward<Args>(args)...);
        }
    };
}