#pragma once
#include <MulNXExtensions/WinExt/Memory/Assembler/Assembler.hpp>
#include <expected>
#include <string>

namespace MulNX {
    namespace Addr {
        class AsmCmdInfo {
        public:
            uint8_t* addr;
            size_t size;
            MulNX::Memory::Asm::Code code;
        };
        class TargetInfo {
        public:
            std::vector<AsmCmdInfo>Cmds;
        };

        // 这个函数要求，至少它分析的确实是一个汇编指令的开头
        TargetInfo AnalyseTarget(uint8_t* target);
        void* TryAlloc(uintptr_t target, size_t size);
        std::expected<MulNX::Memory::Asm::Code, std::string> FixRelativeInstructions(
            const MulNX::Memory::Asm::Code& raw_code, uintptr_t old_base, uintptr_t new_base);
    }
}