#include "Hook.hpp"
#include <Windows.h>
#include <thread>
#include <format>

uintptr_t MulNX::Hook::Dispatch(RegContext* ctx) {
    this->threadNumInAsm.fetch_add(1, std::memory_order_seq_cst);
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
    this->threadNumInAsm.fetch_sub(1, std::memory_order_release);
    return target;
}

void MulNX::Hook::CopyStack(size_t copySize, RegContext* ctx, uintptr_t pCurStack) {
    const int* src = reinterpret_cast<int*>(ctx->rsp + this->frameSize + 0x28); // 0x28 == 0x20(影子空间) + 0x8(call压入的地址)
    void* dst = reinterpret_cast<void*>(pCurStack - 0x8); // 这个0x8是极其特殊地算出来的，注意到进入Asm，首先call压入8，然后push两个压入16，然后注意到影子空间分配是0x28。此时可以分成8 16 8 0x20这样，也就是复制栈不是直接传递过来的rsp，而是再偏移后的8和0x20中间
    memcpy(dst, src, copySize);
}

uint64_t MulNX::Hook::CallMaybeOrigin(size_t copyStackParamNum, RegContext* ctx) {
    using RawCall = uint64_t(*)(MulNX::Hook*, size_t, RegContext*);
    return reinterpret_cast<RawCall>(this->pCallOrigin)(this, (copyStackParamNum) * 8, ctx);
}

std::optional<std::string> MulNX::Hook::BindTarget(uint8_t* target, int len) {
    if (0 < len && len < 5) {
        return std::format("参数指定的长度是：{}  ，这个长度怎么可能放得下一个jmp rel32？？", len);
    }
    if (len == 0) {
        // 自动分析法，info至少提供20个空间
        auto info = MulNX::Addr::AnalyseTarget(target);
        for (int i = 0;len < 5;++i) {
            len += info.Cmds.at(i).size;
        }
    }
    this->hookTarget = target;
    this->overrideSize = len;
    // 复制覆盖处指令
    this->hookTargetRawCode = MulNX::Memory::Asm::Code(target, target + this->overrideSize);
    
    return std::nullopt;
}

std::optional<std::string> MulNX::Hook::CreateAFix() {
    // 修复原始指令（包括jmp call rip相对寻址）
    auto result = MulNX::Addr::FixRelativeInstructions(this->hookTargetRawCode,
        (uintptr_t)this->hookTarget, (uintptr_t)this->pAsmDispatcher + this->dispatcherAsmCode.size());
    if (!result.has_value()) {
        return result.error();
    }

    this->hookTargetFixedCode = result.value();
    return std::nullopt;
}
std::optional<std::string> MulNX::Hook::CreateStart(bool extraStackAdjust, bool callRawFisrt) {
    // 为调度器汇编部分分配空间    
    auto* alloced = MulNX::Addr::TryAlloc((uintptr_t)this->hookTarget, 4096);
    if (!alloced) {
        return "windows内存分配失败！无法找到空间";
    }
    // 进行raii绑定，防止内存泄露
    this->pAsmDispatcher = alloced;
    if (std::abs(static_cast<long long>(reinterpret_cast<uintptr_t>(alloced) -
        reinterpret_cast<uintptr_t>(this->hookTarget))) > 1024ULL * 1024 * 1024) {
        return "windows内存分配失败！分配空间不合适";
    }

    // 创建编译器
    using enum MulNX::Memory::Asm::Reg;
    using namespace MulNX::Memory::Asm;
    Assembler Asm{};

    // 计算结构体大小（保证16字节对齐）
    constexpr size_t ctxSize = (sizeof(RegContext) + 15) & ~15;
    this->frameSize = ctxSize;
    if (!extraStackAdjust)this->frameSize += 8;

    if (callRawFisrt) {
        // 追加原始指令
        if (auto r = this->CreateAFix())return r.value();
        this->dispatcherAsmCode.append_range(this->hookTargetFixedCode);
    }

    Asm
        .sub(RSP, this->frameSize) // 分配栈空间
        .SaveReg();

    this->dispatcherAsmCode.append_range(std::move(Asm.Release()));

    return std::nullopt;
}

std::expected<std::unique_ptr<MulNX::Hook>, std::string> MulNX::Hook::Create(uint8_t* target, std::function<MulNX::Hook::Then(Hook*, RegContext*)>&& callback,
    bool extraStackAdjust, bool callRawFirst, uintptr_t userJmpTarget, int len) {
    // 创建Hook实例
    auto HookInstance = std::make_unique<Hook>();
    HookInstance->callback = std::move(callback);

    if (auto r = HookInstance->BindTarget(target, len))return std::unexpected(r.value());
    if (auto r = HookInstance->CreateStart(extraStackAdjust, callRawFirst))return std::unexpected(r.value());

    // 创建编译器
    using enum MulNX::Memory::Asm::Reg;
    using namespace MulNX::Memory::Asm;
    Assembler Asm{};

    // 指令执行区
    Asm
        .mov(RCX, (uintptr_t)HookInstance.get())// 此参数对应 Hook，是第一个参数
        .mov(RDX, RSP)// 此参数对应 RegContext，是第二个参数
        .mov(RAX, std::bit_cast<uintptr_t>(&MulNX::Hook::Dispatch))// 绑定分发函数
        .sub(RSP, 32)// 分配影子空间
        .call(RAX)// 调用函数，进入CPP语言空间
        .add(RSP, 32)// 回收影子空间
        .jmp(RAX);// 某种意义上，这里是一个分支操作，但是这个分支实际上是由CPP语言层面决定的，因此我们在汇编层面上并不需要区分真假分支，直接让它无条件跳转到jmpTarget即可
    HookInstance->dispatcherAsmCode.append_range(std::move(Asm.Release()));

    // 执行完毕后强制返回被钩函数
    HookInstance->jmpTarget.Return = (uintptr_t)HookInstance->pAsmDispatcher + HookInstance->dispatcherAsmCode.size();
    Asm
        .LoadReg()
        // 释放上下文空间
        .add(RSP, HookInstance->frameSize)
        .ret();// 从分发函数返回

    HookInstance->dispatcherAsmCode.append_range(std::move(Asm.Release()));

    // 执行完毕后执行被覆盖指令并继续执行
    HookInstance->jmpTarget.Continue = (uintptr_t)HookInstance->pAsmDispatcher + HookInstance->dispatcherAsmCode.size();
    Asm
        .LoadReg()
        // 释放上下文空间
        .add(RSP, HookInstance->frameSize);

    HookInstance->dispatcherAsmCode.append_range(std::move(Asm.Release()));

    // 这里恰好可以记录一个可能是原函数地址的位置（如果覆盖的指令是一个完整函数的开头），供回调函数使用
    HookInstance->pMaybeRawFunc = (uintptr_t)HookInstance->pAsmDispatcher + HookInstance->dispatcherAsmCode.size();

    // 追加原始指令
    if (auto r = HookInstance->CreateAFix())return std::unexpected(r.value());
    HookInstance->dispatcherAsmCode.append_range(HookInstance->hookTargetFixedCode);
    // 跳转到原处
    Asm.jmp64((uintptr_t)target + HookInstance->overrideSize);
    HookInstance->dispatcherAsmCode.append_range(std::move(Asm.Release()));

    // 执行完毕后跳过全部被覆盖指令并继续执行
    HookInstance->jmpTarget.SkipAllAndContinue = (uintptr_t)HookInstance->pAsmDispatcher + HookInstance->dispatcherAsmCode.size();
    Asm
        .LoadReg()
        // 释放上下文空间
        .add(RSP, HookInstance->frameSize)
        .jmp64((uintptr_t)target + HookInstance->overrideSize); // 跳转到原处
    HookInstance->dispatcherAsmCode.append_range(std::move(Asm.Release()));

    // 执行完毕后跳到用户指定的位置
    HookInstance->jmpTarget.UserSettedTarget = (uintptr_t)HookInstance->pAsmDispatcher + HookInstance->dispatcherAsmCode.size();
    Asm
        .LoadReg()
        // 释放上下文空间
        .add(RSP, HookInstance->frameSize)
        .jmp64(userJmpTarget); // 跳转到原处
    HookInstance->dispatcherAsmCode.append_range(std::move(Asm.Release()));

    // 栈伪造以调用原函数
    HookInstance->pCallOrigin = (uintptr_t)HookInstance->pAsmDispatcher + HookInstance->dispatcherAsmCode.size();
    // rcx = hook*, rdx = stackCopySize, r8 = regctx*
    Asm
        // 序言
        .push(R12)
        .push(R13)
        //.push(R12)

        .mov(R12, RDX)
        .mov(R13, R8)

        // 环境恢复
        .sub(RSP, R12) // 分配需要的栈空间供参数转移


        // rcx == hook*, rdx == stackCopySize
        .mov(RAX, std::bit_cast<uintptr_t>(&MulNX::Hook::CopyStack)) // 定位栈拷贝函数
        .mov(R9, RSP)
        .sub(RSP, 0x28) // 分配影子空间
        .call(RAX) // 执行栈拷贝

        // 恢复寄存器环境
        .mov(RCX, Mem(R13, offsetof(RegContext, rcx)))
        .mov(RDX, Mem(R13, offsetof(RegContext, rdx)))
        .mov(R8, Mem(R13, offsetof(RegContext, r8)))
        .mov(R9, Mem(R13, offsetof(RegContext, r9)))

        // 调用执行
        .mov(RAX, HookInstance->pMaybeRawFunc) // 定位原始函数
        .call(RAX) // 调用
        .add(RSP, 0x28) // 回收影子空间

        // 重新保存寄存器环境
        .mov(Mem(R13, offsetof(RegContext, rax)), RAX)

        .add(RSP, R12) // 回收拷贝栈空间

        // 尾声
        .pop(R13)
        .pop(R12)
        .ret();

    HookInstance->dispatcherAsmCode.append_range(std::move(Asm.Release()));

    // 复制机器码到VirtualAlloc分配的内存
    memcpy(HookInstance->pAsmDispatcher,
        HookInstance->dispatcherAsmCode.data(),
        HookInstance->dispatcherAsmCode.size());

    // 生成用于覆盖原位置的代码
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

void MulNX::Hook::ResetCallback(std::function<MulNX::Hook::Then(Hook*, RegContext*)>&& callback) {
    this->callback = std::move(callback);
}

MulNX::Hook::Result MulNX::Hook::Attach() {
    __try {
        if (this->attached)return MulNX::Hook::Result::Attached;
        // 覆盖原位置
        DWORD old;
        VirtualProtect(this->hookTarget, this->overrideSize, PAGE_EXECUTE_READWRITE, &old);
        memcpy(this->hookTarget, this->jumperAsmCode.data(), this->overrideSize);
        VirtualProtect(this->hookTarget, this->overrideSize, old, &old);

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
        // 覆盖原位置
        DWORD old;
        VirtualProtect(this->hookTarget, this->overrideSize, PAGE_EXECUTE_READWRITE, &old);
        memcpy(this->hookTarget, this->hookTargetRawCode.data(), this->overrideSize);
        VirtualProtect(this->hookTarget, this->overrideSize, old, &old);

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
    std::this_thread::sleep_for(std::chrono::milliseconds(1));// 这个暂停是为了让汇编执行流有时间进入分派函数，是窗口保护
    while (this->threadNumInAsm.load(std::memory_order_acquire) > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));// 持续等待从回调中离开
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));// 再一个窗口保护让执行流有时间跳回原处，防止还没回去就释放
    VirtualFree(this->pAsmDispatcher, 0, MEM_RELEASE);// 释放空间
}