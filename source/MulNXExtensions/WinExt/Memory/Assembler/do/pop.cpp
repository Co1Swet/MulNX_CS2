#include "../Assembler.hpp"

using namespace MulNX::Memory;

Asm::Assembler& Asm::Assembler::pop(Reg r) {
    auto reg_info = get_reg_info(r);
    if (reg_info.is_ext) {
        emit_byte(0x41);                     // REX.B=1
    }
    emit_byte(0x58 + (reg_info.code & 7));    // POP r64 短编码
    return *this;
}

Asm::Assembler& Asm::Assembler::pop(Mem m) {
    auto reg_info = get_reg_info(m.base);
    if (reg_info.is_ext) {
        emit_byte(0x41);                     // REX.B=1
    }
    emit_byte(0x8F);                           // 操作码
    uint8_t modrm = (0 << 6) | (0 << 3) | (reg_info.code & 7); // mod=00, reg=0 (pop), rm
    emit_byte(modrm);
    return *this;
}