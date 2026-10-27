#include "../Assembler.hpp"

using namespace MulNX::Memory;

Asm::Assembler& Asm::Assembler::SaveReg(uint64_t offset) {
    using enum MulNX::Memory::Asm::Reg;
    // 为了性能考虑，这里可以和上游的rsp调整进行某种配合，这样可以节省cpu周期
    (*this)
        .mov(Mem(RSP, offsetof(RegContext, rax) + offset), RAX)
        .mov(Mem(RSP, offsetof(RegContext, rcx) + offset), RCX)
        .mov(Mem(RSP, offsetof(RegContext, rdx) + offset), RDX)
        .mov(Mem(RSP, offsetof(RegContext, rbx) + offset), RBX)
        .mov(Mem(RSP, offsetof(RegContext, rsp) + offset), RSP) // 保存当前rsp（分配后的）
        .mov(Mem(RSP, offsetof(RegContext, rbp) + offset), RBP)
        .mov(Mem(RSP, offsetof(RegContext, rsi) + offset), RSI)
        .mov(Mem(RSP, offsetof(RegContext, rdi) + offset), RDI)
        .mov(Mem(RSP, offsetof(RegContext, r8) + offset), R8)
        .mov(Mem(RSP, offsetof(RegContext, r9) + offset), R9)
        .mov(Mem(RSP, offsetof(RegContext, r10) + offset), R10)
        .mov(Mem(RSP, offsetof(RegContext, r11) + offset), R11)
        .mov(Mem(RSP, offsetof(RegContext, r12) + offset), R12)
        .mov(Mem(RSP, offsetof(RegContext, r13) + offset), R13)
        .mov(Mem(RSP, offsetof(RegContext, r14) + offset), R14)
        .mov(Mem(RSP, offsetof(RegContext, r15) + offset), R15)
        .movups(Mem(RSP, offsetof(RegContext, xmm0) + offset), Asm::Xmm::XMM0)
        .movups(Mem(RSP, offsetof(RegContext, xmm1) + offset), Asm::Xmm::XMM1)
        .movups(Mem(RSP, offsetof(RegContext, xmm2) + offset), Asm::Xmm::XMM2)
        .movups(Mem(RSP, offsetof(RegContext, xmm3) + offset), Asm::Xmm::XMM3)
        .movups(Mem(RSP, offsetof(RegContext, xmm4) + offset), Asm::Xmm::XMM4)
        .movups(Mem(RSP, offsetof(RegContext, xmm5) + offset), Asm::Xmm::XMM5)
        ;
    return *this;
}

Asm::Assembler& Asm::Assembler::LoadReg(uint64_t offset) {
    using enum MulNX::Memory::Asm::Reg;
    (*this)
        .mov(RAX, Mem(RSP, offsetof(RegContext, rax) + offset))
        .mov(RCX, Mem(RSP, offsetof(RegContext, rcx) + offset))
        .mov(RDX, Mem(RSP, offsetof(RegContext, rdx) + offset))
        .mov(RBX, Mem(RSP, offsetof(RegContext, rbx) + offset))
        // 不恢复 rsp
        .mov(RBP, Mem(RSP, offsetof(RegContext, rbp) + offset))
        .mov(RSI, Mem(RSP, offsetof(RegContext, rsi) + offset))
        .mov(RDI, Mem(RSP, offsetof(RegContext, rdi) + offset))
        .mov(R8, Mem(RSP, offsetof(RegContext, r8) + offset))
        .mov(R9, Mem(RSP, offsetof(RegContext, r9) + offset))
        .mov(R10, Mem(RSP, offsetof(RegContext, r10) + offset))
        .mov(R11, Mem(RSP, offsetof(RegContext, r11) + offset))
        .mov(R12, Mem(RSP, offsetof(RegContext, r12) + offset))
        .mov(R13, Mem(RSP, offsetof(RegContext, r13) + offset))
        .mov(R14, Mem(RSP, offsetof(RegContext, r14) + offset))
        .mov(R15, Mem(RSP, offsetof(RegContext, r15) + offset))
        .movups(Asm::Xmm::XMM0, Mem(RSP, offsetof(RegContext, xmm0) + offset))
        .movups(Asm::Xmm::XMM1, Mem(RSP, offsetof(RegContext, xmm1) + offset))
        .movups(Asm::Xmm::XMM2, Mem(RSP, offsetof(RegContext, xmm2) + offset))
        .movups(Asm::Xmm::XMM3, Mem(RSP, offsetof(RegContext, xmm3) + offset))
        .movups(Asm::Xmm::XMM4, Mem(RSP, offsetof(RegContext, xmm4) + offset))
        .movups(Asm::Xmm::XMM5, Mem(RSP, offsetof(RegContext, xmm5) + offset))
        ;
    return *this;
}