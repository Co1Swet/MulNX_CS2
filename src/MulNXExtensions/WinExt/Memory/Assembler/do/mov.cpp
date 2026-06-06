#include "../Assembler.hpp"

using namespace MulNX::Memory;

Asm::Assembler& Asm::Assembler::mov(Reg dst, Reg src) {
    auto dst_info = get_reg_info(dst);
    auto src_info = get_reg_info(src);

    // 基础 REX 前缀，W=1（64位操作）
    uint8_t rex_byte = 0x48;
    if (src_info.is_ext) rex_byte |= 0x04;  // REX.R
    if (dst_info.is_ext) rex_byte |= 0x01;  // REX.B
    emit_byte(rex_byte);

    emit_byte(0x89); // mov r64, r64 操作码

    uint8_t modrm = 0xC0 | ((src_info.code & 7) << 3) | (dst_info.code & 7);
    emit_byte(modrm);
    return *this;
}
Asm::Assembler& Asm::Assembler::mov(Reg dst, uint64_t imm) {
    auto dst_info = get_reg_info(dst);

    // REX 前缀：W=1（64位操作），如果目标寄存器是扩展寄存器则设置 B 位
    uint8_t rex_byte = 0x48;                     // 基础 REX.W=1
    if (dst_info.is_ext) {
        rex_byte |= 0x01;                         // REX.B 位
    }
    emit_byte(rex_byte);

    // 操作码：0xB8 + 寄存器的低3位
    uint8_t opcode = 0xB8 + (dst_info.code & 7);
    emit_byte(opcode);

    // 立即数（8字节，小端序）
    for (int i = 0; i < 8; ++i) {
        emit_byte((imm >> (i * 8)) & 0xFF);
    }
    return *this;
}

Asm::Assembler& Asm::Assembler::mov(Mem mem, Reg src) {
    auto src_info = get_reg_info(src);          // 源寄存器（出现在 ModRM.reg）
    auto base_info = get_reg_info(mem.base);    // 基址寄存器

    // 确定位移和 mod
    int disp_bytes;
    int mod = get_mod_and_disp(mem.base, mem.disp, disp_bytes);
    bool need_sib = (base_info.code & 7) == 4;  // RSP 或 R12 编码低3位为100

    // 构建 REX 前缀
    uint8_t rex = 0x48; // W=1
    if (src_info.is_ext) rex |= 0x04;           // REX.R
    if (base_info.is_ext) rex |= 0x01;          // REX.B
    // 如果 need_sib 且基址是 R12（扩展），REX.B 已经设置；RSP 不是扩展，所以 REX.B 不会多设
    emit_byte(rex);

    // 操作码 0x89
    emit_byte(0x89);

    // ModRM 字节
    uint8_t modrm = (mod << 6) | ((src_info.code & 7) << 3);
    if (need_sib) {
        modrm |= 0x04; // r/m = 100 表示 SIB 后跟
    }
    else {
        modrm |= (base_info.code & 7);
    }
    emit_byte(modrm);

    // SIB 字节（如果 need_sib）
    if (need_sib) {
        // SIB: base = base_info.code & 7, index = 4 (无索引), scale = 0
        uint8_t sib = (base_info.code & 7) | (4 << 3); // index=4 表示 none
        emit_byte(sib);
    }

    // 位移
    if (disp_bytes == 1) {
        emit_byte(static_cast<uint8_t>(mem.disp & 0xFF));
    }
    else if (disp_bytes == 4) {
        uint32_t disp32 = static_cast<uint32_t>(mem.disp);
        for (int i = 0; i < 4; ++i) {
            emit_byte((disp32 >> (i * 8)) & 0xFF);
        }
    }
    // 如果 disp_bytes == 0，不输出位移

    return *this;
}

Asm::Assembler& Asm::Assembler::mov(Reg dst, Mem mem) {
    auto dst_info = get_reg_info(dst);          // 目标寄存器（出现在 ModRM.reg）
    auto base_info = get_reg_info(mem.base);

    int disp_bytes;
    int mod = get_mod_and_disp(dst, mem.disp, disp_bytes);
    bool need_sib = (base_info.code & 7) == 4;

    uint8_t rex = 0x48;
    if (dst_info.is_ext) rex |= 0x04;           // REX.R
    if (base_info.is_ext) rex |= 0x01;          // REX.B
    emit_byte(rex);

    // 操作码 0x8B
    emit_byte(0x8B);

    uint8_t modrm = (mod << 6) | ((dst_info.code & 7) << 3);
    if (need_sib) {
        modrm |= 0x04;
    }
    else {
        modrm |= (base_info.code & 7);
    }
    emit_byte(modrm);

    if (need_sib) {
        uint8_t sib = (base_info.code & 7) | (4 << 3);
        emit_byte(sib);
    }

    if (disp_bytes == 1) {
        emit_byte(static_cast<uint8_t>(mem.disp & 0xFF));
    }
    else if (disp_bytes == 4) {
        uint32_t disp32 = static_cast<uint32_t>(mem.disp);
        for (int i = 0; i < 4; ++i) {
            emit_byte((disp32 >> (i * 8)) & 0xFF);
        }
    }

    return *this;
}