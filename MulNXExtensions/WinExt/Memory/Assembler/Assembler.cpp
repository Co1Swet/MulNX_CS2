#include "Assembler.hpp"

using namespace MulNX::Memory;

Asm::Code Asm::Assembler::Release() {
    return std::move(this->Asm);
}

Asm::RegInfo Asm::get_reg_info(Reg r) {
    return reg_infos[static_cast<size_t>(r)];
}

// 新签名：需要传入基址寄存器，以处理 RBP/R13 的特殊情况
int Asm::get_mod_and_disp(Reg base, int64_t disp, int& bytes) {
    auto info = get_reg_info(base);
    if (disp == 0) {
        // RBP 或 R13（低 3 位为 5）不能使用 mod=00，必须强制 mod=01 + disp8=0
        if ((info.code & 7) == 5) {
            bytes = 1;
            return 1;   // mod = 01
        }
        bytes = 0;
        return 0;       // mod = 00
    }
    else if (disp >= -128 && disp <= 127) {
        bytes = 1;
        return 1;       // mod = 01
    }
    else {
        bytes = 4;
        return 2;       // mod = 10
    }
}

void Asm::Assembler::emit_byte(uint8_t byte) {
    this->Asm.push_back(byte);
}