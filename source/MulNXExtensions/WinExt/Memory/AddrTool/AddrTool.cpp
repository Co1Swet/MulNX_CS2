#include "AddrTool.hpp"
#include <Windows.h>
#include <Zydis/Zydis.h>

MulNX::Addr::TargetInfo MulNX::Addr::AnalyseTarget(uint8_t* target) {
    ZydisDecoder decoder;
    ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);

    MulNX::Addr::TargetInfo targetInfo{};

    uint8_t* currentBegin = target;
    std::ptrdiff_t currentCmdSize = 1;
    for (;;++currentCmdSize) {
        MulNX::Memory::Asm::Code currentCmd(currentBegin, currentBegin + currentCmdSize);

        ZydisDecodedInstruction instr;
        ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];
        ZyanStatus status = ZydisDecoderDecodeFull(&decoder,
            currentCmd.data(),
            currentCmd.size(),
            &instr, operands);

        if (!ZYAN_SUCCESS(status)) {
            continue;
        }
        else {
            MulNX::Addr::AsmCmdInfo info;
            info.addr = currentBegin;
            info.size = currentCmdSize;
            info.code = currentCmd;
            targetInfo.Cmds.push_back(std::move(info));

            currentBegin += currentCmdSize;
            currentCmdSize = 0;
        }
        if (reinterpret_cast<uintptr_t>(currentBegin) - reinterpret_cast<uintptr_t>(target) > 20) {
            break;
        }
    }
    return targetInfo;
}

void* MulNX::Addr::TryAlloc(uintptr_t target, size_t size) {
    SYSTEM_INFO si;
    GetSystemInfo(&si);

    // 将 DWORD 转换为 uintptr_t 以避免位扩展警告
    uintptr_t allocGran = static_cast<uintptr_t>(si.dwAllocationGranularity); // 通常 64KB
    uintptr_t pageSize = static_cast<uintptr_t>(si.dwPageSize);              // 通常 4KB

    // 将 size 向上对齐到页面大小
    size = (size + pageSize - 1) & ~(pageSize - 1);

    // 搜索范围：target ± 1GB
    const uintptr_t range = 1024ULL * 1024 * 1024;  // 1GB
    uintptr_t startAddr = (target > range) ? (target - range) : 0x10000; // 避免低 64KB
    uintptr_t endAddr = target + range;

    const uintptr_t maxUser = 0x7FFFFFFF0000ULL; // 64 位用户空间上限示例

    if (endAddr > maxUser) endAddr = maxUser;

    // 步长取分配粒度与 1MB 中较大者
    uintptr_t step = std::max(allocGran, static_cast<uintptr_t>(1024 * 1024));

    // 1. 尝试直接在 target 处分配
    LPVOID p = VirtualAlloc(reinterpret_cast<LPVOID>(target), size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (p) return p;

    // 2. 从 target 向低地址和高地址交替尝试
    uintptr_t low = target;
    uintptr_t high = target;

    while (low > startAddr || high < endAddr) {
        // 尝试低地址
        if (low > startAddr) {
            low = (low > step) ? low - step : startAddr;
            // 对齐到分配粒度：注意使用 uintptr_t 类型的掩码
            low = (low + allocGran - 1) & ~(allocGran - 1);
            p = VirtualAlloc(reinterpret_cast<LPVOID>(low), size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
            if (p) return p;
        }

        // 尝试高地址
        if (high < endAddr) {
            high = (high < endAddr - step) ? high + step : endAddr;
            high = high & ~(allocGran - 1);  // 向下对齐
            p = VirtualAlloc(reinterpret_cast<LPVOID>(high), size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
            if (p) return p;
        }
    }

    return nullptr;
}

std::expected<MulNX::Memory::Asm::Code, std::string> MulNX::Addr::FixRelativeInstructions(
    const MulNX::Memory::Asm::Code& raw_code,
    uintptr_t old_base,
    uintptr_t new_base) {


    MulNX::Memory::Asm::Code fixed;
    ZydisDecoder decoder;
    ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);

    size_t offset = 0;
    while (offset < raw_code.size()) {
        ZydisDecodedInstruction instr;
        ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];
        ZyanStatus status = ZydisDecoderDecodeFull(&decoder,
            raw_code.data() + offset,
            raw_code.size() - offset,
            &instr, operands);

        if (!ZYAN_SUCCESS(status)) {
            // 解码失败，直接复制剩余字节
            fixed.insert(fixed.end(), raw_code.begin() + offset, raw_code.end());
            break;
        }

        uintptr_t old_instr_addr = old_base + offset;
        uintptr_t new_instr_addr = new_base + fixed.size();

        // 提取当前指令原始字节
        std::vector<uint8_t> instr_bytes(
            raw_code.begin() + offset,
            raw_code.begin() + offset + instr.length);

        bool handled = false;

        // ---------- 1. 处理相对转移指令 (jmp/call/jcc rel32) ----------
        if (instr.meta.branch_type == ZYDIS_BRANCH_TYPE_NEAR &&
            (instr.attributes & ZYDIS_ATTRIB_IS_RELATIVE)) {
            for (size_t i = 0; i < instr.operand_count; ++i) {
                const auto& op = operands[i];
                if (op.type == ZYDIS_OPERAND_TYPE_IMMEDIATE && op.imm.is_relative) {
                    // 获取该立即数在指令中的偏移和大小（字节）
                    // 注意：instr.raw.imm[i] 与操作数索引 i 对应
                    size_t imm_offset = instr.raw.imm[i].offset;
                    size_t imm_size = instr.raw.imm[i].size / 8;  // 通常为4 (rel32)

                    // 读取原始32位相对偏移（有符号）
                    int32_t orig_disp = 0;
                    memcpy(&orig_disp, raw_code.data() + offset + imm_offset, imm_size);

                    // 原始目标地址 = 当前指令地址 + 指令长度 + 原始偏移
                    uint64_t target = old_instr_addr + instr.length + orig_disp;

                    // 新偏移 = 目标 - (新指令地址 + 指令长度)
                    int64_t new_disp = static_cast<int64_t>(target) - (new_instr_addr + instr.length);

                    if (new_disp < INT32_MIN || new_disp > INT32_MAX) {
                        return std::unexpected("相对跳转偏移超出32位范围，无法修复");
                    }

                    int32_t new_disp32 = static_cast<int32_t>(new_disp);
                    memcpy(instr_bytes.data() + imm_offset, &new_disp32, imm_size);
                    handled = true;
                    break;
                }
            }
        }

        // ---------- 2. 处理 RIP 相对寻址 ----------
        if (!handled) {
            for (size_t i = 0; i < instr.operand_count; ++i) {
                const auto& op = operands[i];
                if (op.type == ZYDIS_OPERAND_TYPE_MEMORY &&
                    op.mem.base == ZYDIS_REGISTER_RIP) {
                    int64_t orig_disp = (op.mem.disp.size != 0) ? op.mem.disp.value : 0;
                    uint64_t target = old_instr_addr + instr.length + orig_disp;
                    int64_t new_disp = target - (new_instr_addr + instr.length);

                    if (new_disp < INT32_MIN || new_disp > INT32_MAX) {
                        return std::unexpected("RIP相对偏移超出32位范围，无法修复");
                    }

                    if (instr.raw.disp.size != 0) {
                        size_t disp_offset = instr.raw.disp.offset;
                        size_t disp_size = instr.raw.disp.size / 8;
                        int32_t disp_to_write = static_cast<int32_t>(new_disp);
                        memcpy(instr_bytes.data() + disp_offset, &disp_to_write, disp_size);
                    }
                    break;
                }
            }
        }

        fixed.insert(fixed.end(), instr_bytes.begin(), instr_bytes.end());
        offset += instr.length;
    }
    return fixed;
}