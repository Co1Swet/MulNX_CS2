#include "Hook.hpp"
#include <MulNX/Config/Config.hpp>
#include <MulNX/Common/Misc.hpp>
#include <Windows.h>
#include <thread>
#include <format>

uintptr_t MulNX::Hook::Dispatch(RegContext* ctx) {
    this->threadNumInAsm.fetch_add(1, std::memory_order_seq_cst);
    auto OnExit = scope_exit([&]() {
        this->threadNumInAsm.fetch_sub(1, std::memory_order_release);
        });
#ifdef _DEBUG
    auto rsp = ctx->rsp;
    auto test = rsp % 16;
    if (test) {
        __debugbreak();
    }
#endif
    auto then = this->callback(this, ctx);
    uint64_t target;
    switch (then) {
    case MulNX::Hook::Then::Return:
        target = this->jmpTarget.Return;
        break;
    case MulNX::Hook::Then::Continue:
        target = this->jmpTarget.Continue;
        break;
    case MulNX::Hook::Then::SkipAllAndContinue:
        target = this->jmpTarget.SkipAllAndContinue;
        break;
    case MulNX::Hook::Then::JmpUserSettedTarget:
        target = this->jmpTarget.UserSettedTarget;
        break;
    default:
        target = this->jmpTarget.Continue;
        break;
    }
    return target;
}

std::expected<void, std::string> MulNX::Hook::BindTarget(uint8_t* target, int len) {
    if (0 < len && len < 5) {
        return std::unexpected(std::format("参数指定的长度是：{} < 5", len));
    }
    if (len <= 0) {
        // info至少提供20个空间
        auto info = MulNX::Addr::AnalyseTarget(target);
        for (int i = 0;len < 5;++i) {
            len += info.Cmds.at(i).size;
        }
    }
    this->hookTarget = target;
    this->overrideSize = len;

    this->hookTargetRawCode = MulNX::Memory::Asm::Code(target, target + this->overrideSize);
    return {};
}
std::expected<MulNX::Memory::Asm::Code, std::string> MulNX::Hook::CreateAFix(size_t RVA) {
    auto result = MulNX::Addr::FixRelativeInstructions(this->hookTargetRawCode,
        (uintptr_t)this->hookTarget, (uintptr_t)this->pAsmDispatcher + RVA);
    if (!result.has_value()) {
        return std::unexpected(result.error());
    }
    return result.value();
}
std::expected<MulNX::Memory::Asm::Assembler, std::string> MulNX::Hook::CreateStart(bool extraStackAdjust, bool callRawFisrt) {
    auto* alloced = MulNX::Addr::TryAlloc((uintptr_t)this->hookTarget, 4096);
    if (!alloced)return std::unexpected("windows内存分配失败！无法找到空间");
    this->pAsmDispatcher = alloced;
    if (std::abs(static_cast<long long>(reinterpret_cast<uintptr_t>(alloced) -
        reinterpret_cast<uintptr_t>(this->hookTarget))) > 1024ULL * 1024 * 1024) {
        return std::unexpected("windows内存分配失败！分配空间不合适");
    }

    using enum MulNX::Memory::Asm::Reg;
    using namespace MulNX::Memory::Asm;
    Assembler Asm{ (uintptr_t)this->pAsmDispatcher };

    constexpr size_t ctxSize = (sizeof(RegContext) + 15) & ~15;
    this->frameSize = ctxSize;
    if (!extraStackAdjust)this->frameSize += 8;

    if (callRawFisrt) {
        if (auto r = this->CreateAFix(Asm.GetCurrentEndRVA())) {
            Asm.Inject(r.value());
        }
        else {
            return std::unexpected(r.error());
        }
    }
    Asm
        .sub(RSP, this->frameSize)
        .SaveReg();

    this->prologueEnd = Asm.GetCurrentEndRVA();
    if (this->prologueEnd > 255) {
        return std::unexpected(std::format("prologue 过长({})，超过 UNWIND_CODE 的 255 偏移上限", this->prologueEnd));
    }

    return Asm;
}

std::expected<std::unique_ptr<MulNX::Hook>, std::string> MulNX::Hook::Create(uint8_t* target, std::move_only_function<MulNX::Hook::Then(Hook*, RegContext*)> callback,
    bool extraStackAdjust, bool callRawFirst, uintptr_t userJmpTarget, int len) {
    if (target == nullptr)MulNX::ErrorTerminate("不能为nullptr创建Hook！");

    auto HookInstance = std::make_unique<Hook>();
    HookInstance->callback = std::move(callback);

    if (auto r = HookInstance->BindTarget(target, len);!r)return std::unexpected(r.error());
    auto rAsm = HookInstance->CreateStart(extraStackAdjust, callRawFirst);
    if (!rAsm)return std::unexpected(rAsm.error());
    auto Asm = std::move(rAsm.value());

    using enum MulNX::Memory::Asm::Reg;
    using namespace MulNX::Memory::Asm;

    Asm
        .mov(RCX, (uintptr_t)HookInstance.get())
        .mov(RDX, RSP)
        .mov(RAX, std::bit_cast<uintptr_t>(&MulNX::Hook::Dispatch))
        .call(RAX);

    HookInstance->jmpRaxOffset = Asm.GetCurrentEndRVA();
    Asm.jmp(RAX);

    HookInstance->jmpTarget.Return = Asm.GetCurrentEndVA();
    Asm
        .LoadReg()
        .add(RSP, HookInstance->frameSize)
        .ret();

    HookInstance->jmpTarget.Continue = Asm.GetCurrentEndVA();
    Asm
        .LoadReg()
        .add(RSP, HookInstance->frameSize);

    HookInstance->pMaybeRawFunc = Asm.GetCurrentEndVA();
    auto rFix = HookInstance->CreateAFix(Asm.GetCurrentEndRVA());
    if (!rFix)return std::unexpected(rFix.error());
    Asm.Inject(rFix.value());
    Asm.jmp64((uintptr_t)target + HookInstance->overrideSize);

    HookInstance->jmpTarget.SkipAllAndContinue = Asm.GetCurrentEndVA();
    Asm
        .LoadReg()
        .add(RSP, HookInstance->frameSize)
        .jmp64((uintptr_t)target + HookInstance->overrideSize);

    HookInstance->jmpTarget.UserSettedTarget = Asm.GetCurrentEndVA();
    Asm
        .LoadReg()
        .add(RSP, HookInstance->frameSize)
        .jmp64(userJmpTarget);
    HookInstance->dispatcherAsmCode.append_range(std::move(Asm.Release()));

    memcpy(HookInstance->pAsmDispatcher,
        HookInstance->dispatcherAsmCode.data(),
        HookInstance->dispatcherAsmCode.size());

    Asm = { (uintptr_t)HookInstance->hookTarget };
    Asm.jmp((uintptr_t)HookInstance->pAsmDispatcher - (uintptr_t)target - 5);
    //Asm.jmp64((uintptr_t)HookInstance->pAsmDispatcher);
    for (int i = 5;i < HookInstance->overrideSize;++i) {
        Asm.nop();
    }
    HookInstance->jumperAsmCode = Asm.Release();

    return HookInstance;
}

void* MulNX::Hook::GetRawStackAddr(RegContext* ctx) {
    return reinterpret_cast<void*>(ctx->rsp + this->frameSize);
}

void MulNX::Hook::ResetCallback(std::move_only_function<MulNX::Hook::Then(Hook*, RegContext*)> callback) {
    this->callback = std::move(callback);
}

MulNX::Hook::Result MulNX::Hook::Attach() {
    __try {
        if (this->attached)return MulNX::Hook::Result::Attached;
        DWORD old{};
        DWORD temp{};
        VirtualProtect(this->hookTarget, this->overrideSize, PAGE_EXECUTE_READWRITE, &old);
        memcpy(this->hookTarget, this->jumperAsmCode.data(), this->overrideSize);
        FlushInstructionCache(GetCurrentProcess(), this->hookTarget, this->overrideSize);
        VirtualProtect(this->hookTarget, this->overrideSize, old, &temp);
        this->attached = true;
        return MulNX::Hook::Result::AttachSuccess;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return MulNX::Hook::Result::AttachError;
    }
}

MulNX::Hook::Result MulNX::Hook::Detach() {
    __try {
        if (!this->attached)return MulNX::Hook::Result::Detached;
        DWORD old{};
        DWORD temp{};
        VirtualProtect(this->hookTarget, this->overrideSize, PAGE_EXECUTE_READWRITE, &old);
        memcpy(this->hookTarget, this->hookTargetRawCode.data(), this->overrideSize);
        FlushInstructionCache(GetCurrentProcess(), this->hookTarget, this->overrideSize);
        VirtualProtect(this->hookTarget, this->overrideSize, old, &temp);
        this->attached = false;
        return MulNX::Hook::Result::DetachSuccess;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return MulNX::Hook::Result::DetachError;
    }
}

MulNX::Hook::~Hook() {
    if (this->pAsmDispatcher == nullptr)return;
    this->Detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    while (this->threadNumInAsm.load(std::memory_order_acquire) > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    if (this->unwindRegistered) {
        RtlDeleteFunctionTable(this->pUnwindTable);
        this->unwindRegistered = false;
    }
    VirtualFree(this->pAsmDispatcher, 0, MEM_RELEASE);
}