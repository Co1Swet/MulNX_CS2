#include "../Assembler.hpp"

using namespace MulNX::Memory;

Asm::Assembler& Asm::Assembler::add(Reg dst, Reg src) {
    // add dst, src 对应 opcode 0x01 (r/m64, r64) 方向：dst 是 r/m，src 是 reg
    // 我们使用 0x01 指令，ModRM.reg = src, r/m = dst
    auto dst_info = get_reg_info(dst);
    auto src_info = get_reg_info(src);

    uint8_t rex = 0x48;
    if (src_info.is_ext) rex |= 0x04; // REX.R
    if (dst_info.is_ext) rex |= 0x01; // REX.B
    emit_byte(rex);

    emit_byte(0x01); // 操作码

    uint8_t modrm = 0xC0; // mod=11 (寄存器直接)
    modrm |= (src_info.code & 7) << 3; // reg 字段 = src
    modrm |= (dst_info.code & 7);      // r/m 字段 = dst
    emit_byte(modrm);
    return *this;
}

Asm::Assembler& Asm::Assembler::add(Reg dst, Mem src) {
    // add dst, [src] 对应 0x03 (r64, r/m64)
    auto dst_info = get_reg_info(dst);
    auto base_info = get_reg_info(src.base);

    int disp_bytes;
    int mod = get_mod_and_disp(dst, src.disp, disp_bytes);
    bool need_sib = (base_info.code & 7) == 4; // RSP/R12

    uint8_t rex = 0x48;
    if (dst_info.is_ext) rex |= 0x04; // REX.R (目标寄存器在 reg 字段)
    if (base_info.is_ext) rex |= 0x01; // REX.B
    emit_byte(rex);

    emit_byte(0x03); // 操作码

    uint8_t modrm = (mod << 6) | ((dst_info.code & 7) << 3);
    if (need_sib) {
        modrm |= 0x04;
    }
    else {
        modrm |= (base_info.code & 7);
    }
    emit_byte(modrm);

    if (need_sib) {
        uint8_t sib = (base_info.code & 7) | (4 << 3); // index=4 (none)
        emit_byte(sib);
    }

    // 位移
    if (disp_bytes == 1) {
        emit_byte(static_cast<uint8_t>(src.disp & 0xFF));
    }
    else if (disp_bytes == 4) {
        uint32_t disp32 = static_cast<uint32_t>(src.disp);
        for (int i = 0; i < 4; ++i) {
            emit_byte((disp32 >> (i * 8)) & 0xFF);
        }
    }
    return *this;
}

Asm::Assembler& Asm::Assembler::add(Mem dst, Reg src) {
    // add [dst], src 对应 0x01 (r/m64, r64)
    auto src_info = get_reg_info(src);
    auto base_info = get_reg_info(dst.base);

    int disp_bytes;
    int mod = get_mod_and_disp(src, dst.disp, disp_bytes);
    bool need_sib = (base_info.code & 7) == 4;

    uint8_t rex = 0x48;
    if (src_info.is_ext) rex |= 0x04; // REX.R (源寄存器在 reg 字段)
    if (base_info.is_ext) rex |= 0x01; // REX.B
    emit_byte(rex);

    emit_byte(0x01); // 操作码

    uint8_t modrm = (mod << 6) | ((src_info.code & 7) << 3);
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
        emit_byte(static_cast<uint8_t>(dst.disp & 0xFF));
    }
    else if (disp_bytes == 4) {
        uint32_t disp32 = static_cast<uint32_t>(dst.disp);
        for (int i = 0; i < 4; ++i) {
            emit_byte((disp32 >> (i * 8)) & 0xFF);
        }
    }
    return *this;
}

Asm::Assembler& Asm::Assembler::add(Reg dst, int32_t imm) {
    auto dst_info = get_reg_info(dst);

    // 判断 imm 是否可以用 8 位有符号表示
    bool use_imm8 = (imm >= -128 && imm <= 127);

    uint8_t rex = 0x48;
    if (dst_info.is_ext) rex |= 0x01; // REX.B
    emit_byte(rex);

    if (use_imm8) {
        emit_byte(0x83); // 操作码
        // ModRM: mod=11, reg=0 (add), r/m=dst
        uint8_t modrm = 0xC0 | (0 << 3) | (dst_info.code & 7);
        emit_byte(modrm);
        emit_byte(static_cast<uint8_t>(imm & 0xFF));
    }
    else {
        emit_byte(0x81); // 操作码
        uint8_t modrm = 0xC0 | (0 << 3) | (dst_info.code & 7);
        emit_byte(modrm);
        // 32 位立即数，小端序
        uint32_t imm32 = static_cast<uint32_t>(imm);
        for (int i = 0; i < 4; ++i) {
            emit_byte((imm32 >> (i * 8)) & 0xFF);
        }
    }
    return *this;
}

Asm::Assembler& Asm::Assembler::add(Mem dst, int32_t imm) {
    auto base_info = get_reg_info(dst.base);
    bool use_imm8 = (imm >= -128 && imm <= 127);

    int disp_bytes;
    int mod = get_mod_and_disp(dst.base, dst.disp, disp_bytes);
    bool need_sib = (base_info.code & 7) == 4;

    uint8_t rex = 0x48;
    if (base_info.is_ext) rex |= 0x01; // REX.B
    emit_byte(rex);

    emit_byte(use_imm8 ? 0x83 : 0x81); // 操作码

    uint8_t modrm = (mod << 6) | (0 << 3); // reg=0 (add)
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

    // 位移
    if (disp_bytes == 1) {
        emit_byte(static_cast<uint8_t>(dst.disp & 0xFF));
    }
    else if (disp_bytes == 4) {
        uint32_t disp32 = static_cast<uint32_t>(dst.disp);
        for (int i = 0; i < 4; ++i) {
            emit_byte((disp32 >> (i * 8)) & 0xFF);
        }
    }

    // 立即数
    if (use_imm8) {
        emit_byte(static_cast<uint8_t>(imm & 0xFF));
    }
    else {
        uint32_t imm32 = static_cast<uint32_t>(imm);
        for (int i = 0; i < 4; ++i) {
            emit_byte((imm32 >> (i * 8)) & 0xFF);
        }
    }
    return *this;
}
