#include "../Assembler.hpp"

using namespace MulNX::Memory;

Asm::Assembler& Asm::Assembler::nop() {
    Begin();
    emit_byte(0x90);
    return End();
}