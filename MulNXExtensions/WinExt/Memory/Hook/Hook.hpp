#pragma once

#include <MulNXExtensions/WinExt/Memory/Assembler/Assembler.hpp>
#include <functional>
#include <expected>
#include <atomic>
#include <memory>
#include <string>

class RegContext {
public:
    uint64_t rax, rcx, rdx, rbx, rsp, rbp, rsi, rdi;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
};

namespace MulNX {
    class AsmCmdInfo {
    public:
        uint8_t* addr;
        size_t size;
        MulNX::Memory::Asm::Code code;
    };
    class HookTargetInfo {
    public:
        std::vector<AsmCmdInfo>Cmds;
    };

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
            Return
        };
    private:
        const static size_t allocSize = 1000;

        bool attached = false;
        std::function<Then(RegContext*, Hook*)> callback;
        std::atomic<size_t> threadNumInAsm = 0;

        void* pAsmDispatcher = nullptr;
        MulNX::Memory::Asm::Code dispatcherAsmCode{};

        HookTargetInfo hookTargetInfo{};
        uint8_t* hookTarget = nullptr;
        size_t overrideSize = 0;
        MulNX::Memory::Asm::Code hookTargetRawCode{};
        MulNX::Memory::Asm::Code jumperAsmCode{};


    private:
        static std::expected<MulNX::Memory::Asm::Code, std::string> FixRelativeInstructions(const MulNX::Memory::Asm::Code& raw_code,
            uintptr_t old_base, uintptr_t new_base);
        uintptr_t Dispatch(RegContext* ctx);
        // 这个函数要求，至少它分析的确实是一个汇编指令的开头
        static HookTargetInfo AnalyseTarget(uint8_t* target);

        uintptr_t jmpForReturn = 0;
        uintptr_t jmpForContinue = 0;
    public:
        size_t frameSize = 0;
        uintptr_t pMaybeRawFunc = 0;// 可能的原函数地址（如果覆盖的指令是一个完整函数的开头）
        Hook() = default;
        ~Hook();

        // 通过frameSize得到存在原始栈上的参数，而非被重新分配的栈上的参数
        void* GetRawStackAddr(RegContext* ctx);

        // 关于栈调整参数，当其为false时，模拟原始栈状态进行回调；当其为true时，则认为栈状态非16字节对齐，内部进行对齐操作（常常是函数中间Hook）
        static std::expected<std::unique_ptr<Hook>, std::string> Create(uint8_t* Target, int Len, bool extraStackAdjust, std::function<Then(RegContext*, Hook*)>&& callback);
        Result Attach();
        Result Detach();
    };
}