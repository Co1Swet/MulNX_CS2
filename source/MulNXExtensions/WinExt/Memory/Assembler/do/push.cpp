#include "../Assembler.hpp"

using namespace MulNX::Memory;

Asm::Assembler& Asm::Assembler::push(Reg r) {
    auto reg_info = get_reg_info(r);
    if (reg_info.is_ext) {
        emit_byte(0x41);                     // REX.B=1
    }
    emit_byte(0x50 + (reg_info.code & 7));    // PUSH r64 短编码
    return *this;
}

Asm::Assembler& Asm::Assembler::push(Mem m) {
    auto reg_info = get_reg_info(m.base);
    if (reg_info.is_ext) {
        emit_byte(0x41);                     // REX.B=1
    }
    emit_byte(0xFF);                           // 操作码
    uint8_t modrm = (0 << 6) | (6 << 3) | (reg_info.code & 7); // mod=00, reg=6 (push), rm
    emit_byte(modrm);
    return *this;
}