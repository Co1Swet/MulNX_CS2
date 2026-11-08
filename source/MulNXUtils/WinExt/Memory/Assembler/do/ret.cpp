#include "../Assembler.hpp"

using namespace MulNX::Memory;

Asm::Assembler& Asm::Assembler::ret() {
    Begin();
    emit_byte(0xC3);
    return End();
}