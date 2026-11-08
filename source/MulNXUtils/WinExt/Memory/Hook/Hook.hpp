#pragma once
#include <MulNX/Common/Type.hpp>
#include <MulNX/Common/Exception.hpp>
#include <MulNXUtils/WinExt/Memory/AddrTool/AddrTool.hpp>
#include <MulNXUtils/WinExt/Memory/Assembler/Assembler.hpp>
#include <Windows.h>
#include <atomic>
#include <bit>
#include <optional>
#include <expected>
#include <functional>
#include <memory>
#include <source_location>

namespace MulNX {
    class Hook final {
    public:
        enum class Result : uint8_t {
            Attached, AttachSuccess, AttachError,
            Detached, DetachSuccess, DetachError
        };

        enum class Then {
            Continue, SkipAllAndContinue, JmpUserSettedTarget, Return
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
        MulNX::Memory::Asm::Code jumperAsmCode{};

        struct JmpTarget {
            uintptr_t Return = 0;
            uintptr_t Continue = 0;
            uintptr_t SkipAllAndContinue = 0;
            uintptr_t UserSettedTarget = 0;
        };
        JmpTarget jmpTarget{};
        size_t frameSize = 0;

        // UNWIND 相关
        RUNTIME_FUNCTION* pUnwindTable = nullptr;
        size_t subEndRVA = 0;   // sub rsp, frameSize 结束 RVA（UNWIND_CODE.CodeOffset & SizeOfProlog）
        size_t prologueEnd = 0;   // SaveReg 结束后 RVA
        size_t jmpRaxOffset = 0;   // jmp rax 起始 RVA
        bool   unwindRegistered = false;

        uintptr_t Dispatch(RegContext* ctx);
        std::expected<void, std::string> BindTarget(uint8_t* target, int len);
        std::expected<MulNX::Memory::Asm::Code, std::string> CreateAFix(size_t RVA);
        std::expected<MulNX::Memory::Asm::Assembler, std::string> CreateStart(bool extraStackAdjust, bool callRawFisrt);
        std::expected<void, std::string> RegisterUnwindInfo();
    public:
        uintptr_t pMaybeRawFunc = 0;

        ~Hook();

        struct CreateOptions {
            bool callRawFirst = false;
            uintptr_t userJmpTarget = 0;
            int len = 0;
        };

        static std::expected<std::unique_ptr<Hook>, MulNX::Exception> Create(
            uint8_t* target, std::move_only_function<MulNX::Hook::Then(Hook*, RegContext*)> callback,
            bool extraStackAdjust = false, CreateOptions opts = {},
            std::source_location loc = std::source_location::current());

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