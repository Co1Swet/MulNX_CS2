#include "../Assembler.hpp"

using namespace MulNX::Memory;

Asm::Assembler& Asm::Assembler::call(Reg r) {
    auto reg_info = get_reg_info(r);
    if (reg_info.is_ext) {
        emit_byte(0x41); // REX.B=1 (处理 R8~R15)
    }
    emit_byte(0xFF);      // 操作码
    // ModRM: mod=11 (寄存器直接), reg=010 (call), r/m=寄存器的低3位
    uint8_t modrm = (3 << 6) | (2 << 3) | (reg_info.code & 7);
    emit_byte(modrm);
    return *this;
}
Asm::Assembler& Asm::Assembler::call(Mem m) {
    auto reg_info = get_reg_info(m.base);

    // 如果基址是扩展寄存器（R8-R15），需要添加 REX 前缀（REX.B=1）
    if (reg_info.is_ext) {
        // REX 前缀：0100 0001 (0x41) 表示 REX.W=0, REX.R=0, REX.X=0, REX.B=1
        emit_byte(0x41);
    }

    // 操作码 0xFF
    emit_byte(0xFF);

    // ModRM 字节：mod=00 (无位移), reg=010 (表示 call 操作), rm=基址寄存器的低3位
    uint8_t modrm = (0 << 6) | (2 << 3) | (reg_info.code & 7);
    emit_byte(modrm);

    return *this;
}