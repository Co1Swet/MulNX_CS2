#include "../Assembler.hpp"

using namespace MulNX::Memory;

Asm::Assembler& Asm::Assembler::SaveReg() {
    using enum MulNX::Memory::Asm::Reg;
    // 保存所有寄存器到 [rsp + offset]
    (*this)
        .mov(Mem(RSP, offsetof(RegContext, rax)), RAX)
        .mov(Mem(RSP, offsetof(RegContext, rcx)), RCX)
        .mov(Mem(RSP, offsetof(RegContext, rdx)), RDX)
        .mov(Mem(RSP, offsetof(RegContext, rbx)), RBX)
        .mov(Mem(RSP, offsetof(RegContext, rsp)), RSP) // 保存当前rsp（分配后的）
        .mov(Mem(RSP, offsetof(RegContext, rbp)), RBP)
        .mov(Mem(RSP, offsetof(RegContext, rsi)), RSI)
        .mov(Mem(RSP, offsetof(RegContext, rdi)), RDI)
        .mov(Mem(RSP, offsetof(RegContext, r8)), R8)
        .mov(Mem(RSP, offsetof(RegContext, r9)), R9)
        .mov(Mem(RSP, offsetof(RegContext, r10)), R10)
        .mov(Mem(RSP, offsetof(RegContext, r11)), R11)
        .mov(Mem(RSP, offsetof(RegContext, r12)), R12)
        .mov(Mem(RSP, offsetof(RegContext, r13)), R13)
        .mov(Mem(RSP, offsetof(RegContext, r14)), R14)
        .mov(Mem(RSP, offsetof(RegContext, r15)), R15)
        ;
    return *this;
}

Asm::Assembler& Asm::Assembler::LoadReg() {
    using enum MulNX::Memory::Asm::Reg;
    (*this)
        .mov(RAX, Mem(RSP, offsetof(RegContext, rax)))
        .mov(RCX, Mem(RSP, offsetof(RegContext, rcx)))
        .mov(RDX, Mem(RSP, offsetof(RegContext, rdx)))
        .mov(RBX, Mem(RSP, offsetof(RegContext, rbx)))
        // 不恢复 rsp
        .mov(RBP, Mem(RSP, offsetof(RegContext, rbp)))
        .mov(RSI, Mem(RSP, offsetof(RegContext, rsi)))
        .mov(RDI, Mem(RSP, offsetof(RegContext, rdi)))
        .mov(R8, Mem(RSP, offsetof(RegContext, r8)))
        .mov(R9, Mem(RSP, offsetof(RegContext, r9)))
        .mov(R10, Mem(RSP, offsetof(RegContext, r10)))
        .mov(R11, Mem(RSP, offsetof(RegContext, r11)))
        .mov(R12, Mem(RSP, offsetof(RegContext, r12)))
        .mov(R13, Mem(RSP, offsetof(RegContext, r13)))
        .mov(R14, Mem(RSP, offsetof(RegContext, r14)))
        .mov(R15, Mem(RSP, offsetof(RegContext, r15)))
        ;
    return *this;
}