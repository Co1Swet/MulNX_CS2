#pragma once
#include <MulNX/Common/Type.hpp>
#include <MulNXUtils/WinExt/Memory/AddrTool/AddrTool.hpp>
#include <MulNXUtils/WinExt/Memory/Assembler/Assembler.hpp>
#include <Windows.h>
#include <atomic>
#include <bit>
#include <optional>
#include <expected>
#include <functional>
#include <memory>

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
        std::move_only_function<Then(Hook*, RegContext*)> callback;
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

        RUNTIME_FUNCTION* pUnwindTable = nullptr;
        size_t prologueEnd = 0;
        size_t jmpRaxOffset = 0;
        bool unwindRegistered = false;

        uintptr_t Dispatch(RegContext* ctx);
        std::expected<void, std::string> BindTarget(uint8_t* target, int len);
        std::expected<void, std::string> CreateAFix();
        std::expected<void, std::string> CreateStart(bool extraStackAdjust, bool callRawFisrt);
    public:
        uintptr_t pMaybeRawFunc = 0;

        ~Hook();

        static std::expected<std::unique_ptr<Hook>, std::string> Create(
            uint8_t* target, std::move_only_function<MulNX::Hook::Then(Hook*, RegContext*)> callback,
            bool extraStackAdjust = false, bool callRawFisrt = false,
            uintptr_t userJmpTarget = 0, int len = 0);

        Result Attach();
        Result Detach();

        auto GetHookTarget()const { return this->hookTarget; }
        void ResetCallback(std::move_only_function<MulNX::Hook::Then(Hook*, RegContext*)> callback);

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