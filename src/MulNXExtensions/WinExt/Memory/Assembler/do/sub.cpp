#include "../Assembler.hpp"

using namespace MulNX::Memory;

Asm::Assembler& Asm::Assembler::sub(Reg dst, Reg src) {
    auto dst_info = get_reg_info(dst);
    auto src_info = get_reg_info(src);

    uint8_t rex = 0x48;
    if (src_info.is_ext) rex |= 0x04;
    if (dst_info.is_ext) rex |= 0x01;
    emit_byte(rex);

    emit_byte(0x29); // sub r/m64, r64 操作码
    uint8_t modrm = 0xC0;
    modrm |= (src_info.code & 7) << 3;
    modrm |= (dst_info.code & 7);
    emit_byte(modrm);
    return *this;
}

Asm::Assembler& Asm::Assembler::sub(Reg dst, Mem src) {
    // sub dst, [src] 对应 0x2B
    auto dst_info = get_reg_info(dst);
    auto base_info = get_reg_info(src.base);

    int disp_bytes;
    int mod = get_mod_and_disp(dst, src.disp, disp_bytes);
    bool need_sib = (base_info.code & 7) == 4;

    uint8_t rex = 0x48;
    if (dst_info.is_ext) rex |= 0x04;
    if (base_info.is_ext) rex |= 0x01;
    emit_byte(rex);

    emit_byte(0x2B); // 操作码

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

Asm::Assembler& Asm::Assembler::sub(Mem dst, Reg src) {
    // sub [dst], src 对应 0x29
    auto src_info = get_reg_info(src);
    auto base_info = get_reg_info(dst.base);

    int disp_bytes;
    int mod = get_mod_and_disp(dst.base, dst.disp, disp_bytes);
    bool need_sib = (base_info.code & 7) == 4;

    uint8_t rex = 0x48;
    if (src_info.is_ext) rex |= 0x04;
    if (base_info.is_ext) rex |= 0x01;
    emit_byte(rex);

    emit_byte(0x29); // 操作码

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

Asm::Assembler& Asm::Assembler::sub(Reg dst, int32_t imm) {
    auto dst_info = get_reg_info(dst);
    bool use_imm8 = (imm >= -128 && imm <= 127);

    uint8_t rex = 0x48;
    if (dst_info.is_ext) rex |= 0x01;
    emit_byte(rex);

    if (use_imm8) {
        emit_byte(0x83); // 操作码
        uint8_t modrm = 0xC0 | (5 << 3) | (dst_info.code & 7); // reg=5 (sub)
        emit_byte(modrm);
        emit_byte(static_cast<uint8_t>(imm & 0xFF));
    }
    else {
        emit_byte(0x81); // 操作码
        uint8_t modrm = 0xC0 | (5 << 3) | (dst_info.code & 7);
        emit_byte(modrm);
        uint32_t imm32 = static_cast<uint32_t>(imm);
        for (int i = 0; i < 4; ++i) {
            emit_byte((imm32 >> (i * 8)) & 0xFF);
        }
    }
    return *this;
}

Asm::Assembler& Asm::Assembler::sub(Mem dst, int32_t imm) {
    auto base_info = get_reg_info(dst.base);
    bool use_imm8 = (imm >= -128 && imm <= 127);

    int disp_bytes;
    int mod = get_mod_and_disp(dst.base, dst.disp, disp_bytes);
    bool need_sib = (base_info.code & 7) == 4;

    uint8_t rex = 0x48;
    if (base_info.is_ext) rex |= 0x01;
    emit_byte(rex);

    emit_byte(use_imm8 ? 0x83 : 0x81);

    uint8_t modrm = (mod << 6) | (5 << 3); // reg=5 (sub)
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