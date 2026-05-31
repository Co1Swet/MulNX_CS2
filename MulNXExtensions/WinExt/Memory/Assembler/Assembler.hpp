#pragma once

#include <vector>
#include <cstdint>

class RegContext {
public:
    uint64_t rax, rcx, rdx, rbx, rsp, rbp, rsi, rdi;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
};

namespace MulNX {
    namespace Memory {
        namespace Asm {
            enum class Reg : uint8_t {
                RAX, RCX, RDX, RBX, RSP, RBP, RSI, RDI,
                R8, R9, R10, R11, R12, R13, R14, R15
            };

            class Mem {
            public:
                Reg base;        // 基址寄存器
                int64_t disp;    // 位移量（可以为 0）
                Mem() = delete;
                Mem(Reg base, int64_t disp = 0) : base(base), disp(disp) {}
            };


            // 根据位移量确定 mod 字段和位移大小（返回 mod 值，并通过 bytes 输出位移字节数）
            int get_mod_and_disp(Reg base, int64_t disp, int& bytes);
            struct RegInfo {
                uint8_t code;    // 硬件编码 (0-15)
                bool is_ext;     // 是否扩展寄存器 (R8-R15)
            };

            constexpr RegInfo reg_infos[] = {
                {0, false},  // RAX
                {1, false},  // RCX
                {2, false},  // RDX
                {3, false},  // RBX
                {4, false},  // RSP
                {5, false},  // RBP
                {6, false},  // RSI
                {7, false},  // RDI
                {8, true},   // R8
                {9, true},   // R9
                {10, true},  // R10
                {11, true},  // R11
                {12, true},  // R12
                {13, true},  // R13
                {14, true},  // R14
                {15, true},  // R15
            };

            RegInfo get_reg_info(Reg r);

            class Code final :public std::vector<uint8_t> {
            public:
                using std::vector<uint8_t>::vector;
            };

            class Assembler {
                Code Asm{};
                void emit_byte(uint8_t byte);
            public:
                // mov 指令
                Assembler& mov(Reg R1, Reg R2);
                Assembler& mov(Mem M1, Reg R2);
                Assembler& mov(Reg R1, Mem M2);
                Assembler& mov(Reg dst, uint64_t imm);

                // call 指令
                Assembler& call(Reg R);
                Assembler& call(Mem R);
                Assembler& nop();

                Assembler& ret();

                Assembler& push(Reg r);
                Assembler& push(Mem m);
                Assembler& pop(Reg r);
                Assembler& pop(Mem m);
                Assembler& jmp(Reg r);
                Assembler& jmp(Mem m);
                Assembler& jmp(int32_t rel_offset);      // JMP rel32（近跳转，偏移量有符号32位）
                Assembler& jmp_rel8(int8_t rel_offset);  // JMP rel8（短跳转，偏移量有符号8位）
                Assembler& jmp64(uint64_t target);   // 生成绝对跳转

                // add 指令
                Assembler& add(Reg dst, Reg src);               // add dst, src
                Assembler& add(Reg dst, Mem src);                // add dst, [src]
                Assembler& add(Mem dst, Reg src);                // add [dst], src
                Assembler& add(Reg dst, int32_t imm);            // add dst, imm (符号扩展)
                Assembler& add(Mem dst, int32_t imm);            // add [dst], imm (符号扩展)

                // sub 指令
                Assembler& sub(Reg dst, Reg src);               // sub dst, src
                Assembler& sub(Reg dst, Mem src);                // sub dst, [src]
                Assembler& sub(Mem dst, Reg src);                // sub [dst], src
                Assembler& sub(Reg dst, int32_t imm);            // sub dst, imm (符号扩展)
                Assembler& sub(Mem dst, int32_t imm);            // sub [dst], imm (符号扩展)

                Assembler& SaveReg();
                Assembler& LoadReg();

                Code Release();
            };
        }
    }
}