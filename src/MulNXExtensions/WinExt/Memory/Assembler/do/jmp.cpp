#include "../Assembler.hpp"

using namespace MulNX::Memory;

Asm::Assembler& Asm::Assembler::jmp(Reg r) {
    auto reg_info = get_reg_info(r);
    if (reg_info.is_ext) {
        emit_byte(0x41);                     // REX.B=1
    }
    emit_byte(0xFF);                           // 操作码
    uint8_t modrm = (3 << 6) | (4 << 3) | (reg_info.code & 7); // mod=11, reg=4 (jmp), rm
    emit_byte(modrm);
    return *this;
}

Asm::Assembler& Asm::Assembler::jmp(Mem m) {
    auto reg_info = get_reg_info(m.base);
    if (reg_info.is_ext) {
        emit_byte(0x41);                     // REX.B=1
    }
    emit_byte(0xFF);                           // 操作码
    uint8_t modrm = (0 << 6) | (4 << 3) | (reg_info.code & 7); // mod=00, reg=4 (jmp), rm
    emit_byte(modrm);
    return *this;
}

// JMP rel32：操作码 0xE9 + 4字节偏移（小端序）
Asm::Assembler& Asm::Assembler::jmp(int32_t rel_offset) {
    emit_byte(0xE9); // 近跳转操作码
    // 写入4字节有符号偏移量（小端序）
    for (int i = 0; i < 4; ++i) {
        emit_byte((rel_offset >> (i * 8)) & 0xFF);
    }
    return *this;
}

// JMP rel8：操作码 0xEB + 1字节偏移
Asm::Assembler& Asm::Assembler::jmp_rel8(int8_t rel_offset) {
    emit_byte(0xEB); // 短跳转操作码
    emit_byte(static_cast<uint8_t>(rel_offset));
    return *this;
}
Asm::Assembler& Asm::Assembler::jmp64(uint64_t target) {
    // 生成 jmp qword ptr [rip+0] : FF 25 00 00 00 00
    emit_byte(0xFF);           // 操作码
    emit_byte(0x25);           // ModRM: mod=00, reg=4 (jmp), rm=101 (RIP相对)
    emit_byte(0x00);           // 偏移量低字节 (disp32 = 0)
    emit_byte(0x00);
    emit_byte(0x00);
    emit_byte(0x00);
    // 将目标地址作为 8 字节立即数附加在指令后
    for (int i = 0; i < 8; ++i) {
        emit_byte((target >> (i * 8)) & 0xFF);
    }
    return *this;
}