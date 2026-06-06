#include "../Assembler.hpp"

using namespace MulNX::Memory;

Asm::Assembler& Asm::Assembler::ret() {
    emit_byte(0xC3);
    return *this;
}