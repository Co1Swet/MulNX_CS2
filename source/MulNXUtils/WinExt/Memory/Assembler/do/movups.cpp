#include "../Assembler.hpp"

using namespace MulNX::Memory;

// movups [mem], xmm  ->  0F 11 /r
Asm::Assembler& Asm::Assembler::movups(Mem m, Xmm src) {
    // XMM 的硬件编码和 GPR 完全一致（0~15），直接复用 reg_infos
    auto src_info = reg_infos[static_cast<size_t>(src)];
    auto base_info = get_reg_info(m.base);

    // 注意：第一个参数必须传 m.base，不要传成 src
    int disp_bytes = 0;
    int mod = get_mod_and_disp(m.base, m.disp, disp_bytes);
    bool need_sib = (base_info.code & 7) == 4;  // RSP / R12

    // REX：W=0，只有 XMM8~15 或 R8~R15 基址才需要
    uint8_t rex = 0x40;
    if (src_info.is_ext)  rex |= 0x04;  // REX.R：XMM8~15
    if (base_info.is_ext) rex |= 0x01;  // REX.B：R8~R15 基址
    if (rex != 0x40) emit_byte(rex);    // 不需要时省略整个 REX 前缀

    emit_byte(0x0F);
    emit_byte(0x11);                    // movups 存储

    // ModRM：mod | reg=XMM低3位 | r/m=基址或 SIB 标记
    uint8_t modrm = static_cast<uint8_t>(mod << 6)
        | static_cast<uint8_t>((src_info.code & 7) << 3);
    if (need_sib) {
        modrm |= 0x04;
    }
    else {
        modrm |= (base_info.code & 7);
    }
    emit_byte(modrm);

    // SIB：base=基址低3位，index=4（无索引），scale=0
    if (need_sib) {
        uint8_t sib = (base_info.code & 7) | (4 << 3);
        emit_byte(sib);
    }

    // 位移
    if (disp_bytes == 1) {
        emit_byte(static_cast<uint8_t>(m.disp & 0xFF));
    }
    else if (disp_bytes == 4) {
        uint32_t disp32 = static_cast<uint32_t>(m.disp);
        for (int i = 0; i < 4; ++i) {
            emit_byte((disp32 >> (i * 8)) & 0xFF);
        }
    }

    return *this;
}

// movups xmm, [mem]  ->  0F 10 /r
Asm::Assembler& Asm::Assembler::movups(Xmm dst, Mem m) {
    auto dst_info = reg_infos[static_cast<size_t>(dst)];
    auto base_info = get_reg_info(m.base);

    int disp_bytes = 0;
    int mod = get_mod_and_disp(m.base, m.disp, disp_bytes);
    bool need_sib = (base_info.code & 7) == 4;

    uint8_t rex = 0x40;
    if (dst_info.is_ext)  rex |= 0x04;  // REX.R
    if (base_info.is_ext) rex |= 0x01;  // REX.B
    if (rex != 0x40) emit_byte(rex);

    emit_byte(0x0F);
    emit_byte(0x10);                    // movups 加载

    uint8_t modrm = static_cast<uint8_t>(mod << 6)
        | static_cast<uint8_t>((dst_info.code & 7) << 3);
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
        emit_byte(static_cast<uint8_t>(m.disp & 0xFF));
    }
    else if (disp_bytes == 4) {
        uint32_t disp32 = static_cast<uint32_t>(m.disp);
        for (int i = 0; i < 4; ++i) {
            emit_byte((disp32 >> (i * 8)) & 0xFF);
        }
    }

    return *this;
}