#include "../Assembler.hpp"

using namespace MulNX::Memory;

Asm::Assembler& Asm::Assembler::nop() {
    emit_byte(0x90);
    return *this;
}