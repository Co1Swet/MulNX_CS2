#pragma once
#include <Windows.h>
#include <string>
#include <format>

// ─────────────────────────────────────────────────────────────
// 诊断辅助：描述一个地址的所属段 / 模块
// 返回示例：
//   "IMAGE:networksystem.dll+0x466b90 (COMMIT,XR)"
//   "MAPPED:base=0x16c500000000 sz=0x100000 (COMMIT,XR)"
//   "PRIVATE:base=0x16c508300000 sz=0x1000 (COMMIT,RW)"
//   "VQ-fail(gle=487)"
// ─────────────────────────────────────────────────────────────
std::string DescribeAddress(uint64_t addr) {
    if (!addr) return "null";

    MEMORY_BASIC_INFORMATION mbi = {};
    if (!VirtualQuery((LPCVOID)addr, &mbi, sizeof(mbi))) {
        return std::format("VQ-fail(gle={})", GetLastError());
    }

    const char* typeStr = "?";
    switch (mbi.Type) {
    case MEM_IMAGE:   typeStr = "IMAGE";   break;
    case MEM_MAPPED:  typeStr = "MAPPED";  break;
    case MEM_PRIVATE: typeStr = "PRIVATE"; break;
    default:          typeStr = "FREE";    break;
    }

    const char* stateStr = "?";
    switch (mbi.State) {
    case MEM_COMMIT:  stateStr = "COMMIT";  break;
    case MEM_RESERVE: stateStr = "RESERVE"; break;
    case MEM_FREE:    stateStr = "FREE";    break;
    }

    const char* protStr = "?";
    switch (mbi.Protect) {
    case PAGE_EXECUTE_READ:        protStr = "XR";  break;
    case PAGE_EXECUTE_READWRITE:   protStr = "XRW"; break;
    case PAGE_EXECUTE:             protStr = "X";   break;
    case PAGE_EXECUTE_WRITECOPY:   protStr = "XWC"; break;
    case PAGE_READONLY:            protStr = "R";   break;
    case PAGE_READWRITE:           protStr = "RW";  break;
    case PAGE_WRITECOPY:           protStr = "WC";  break;
    case PAGE_NOACCESS:            protStr = "NA";  break;
    default:                       protStr = "?";   break;
    }

    // IMAGE：能反查模块名 + RVA
    if (mbi.Type == MEM_IMAGE) {
        HMODULE hMod = nullptr;
        if (GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCWSTR)mbi.AllocationBase, &hMod) && hMod) {
            char nameA[MAX_PATH] = {};
            if (GetModuleFileNameA(hMod, nameA, MAX_PATH)) {
                const char* slash = strrchr(nameA, '\\');
                const char* modName = slash ? slash + 1 : nameA;
                return std::format("{}:{}+{:#x} ({},{})",
                    typeStr, modName,
                    addr - (uint64_t)mbi.AllocationBase,
                    stateStr, protStr);
            }
        }
        return std::format("{}:base={:#x}+{:#x} ({},{})",
            typeStr, (uint64_t)mbi.AllocationBase,
            addr - (uint64_t)mbi.AllocationBase,
            stateStr, protStr);
    }

    // MAPPED / PRIVATE：只能给 base + size
    return std::format("{}:base={:#x} sz={:#x} ({},{})",
        typeStr, (uint64_t)mbi.BaseAddress,
        (uint64_t)mbi.RegionSize,
        stateStr, protStr);
}