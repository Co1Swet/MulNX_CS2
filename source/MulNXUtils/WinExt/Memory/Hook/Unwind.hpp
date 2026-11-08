#pragma once
#include <cstdint>

// ============================================================
// x64 异常处理结构（依据 Windows 官方文档）
//
// 用户态 Windows SDK 的 winnt.h 只提供：
//   - RUNTIME_FUNCTION / PRUNTIME_FUNCTION
//   - RtlAddFunctionTable / RtlDeleteFunctionTable / RtlVirtualUnwind
//   - UNW_FLAG_EHANDLER / UNW_FLAG_UHANDLER / UNW_FLAG_CHAININFO
// 但不提供 UNWIND_CODE / UNWIND_INFO / UWOP_* 枚举。
// 这里按官方 C 定义给出最小兼容版本，用移位替代位域以规避编译器差异。
// ============================================================

union MulNX_UNWIND_CODE {
    struct {
        uint8_t CodeOffset;      // prolog 中该指令结束处的偏移
        uint8_t UnwindOpInfo;    // 低 4 位 = UnwindOp, 高 4 位 = OpInfo
    } s;
    uint16_t FrameOffset;        // ExtraData 通道
};
static_assert(sizeof(MulNX_UNWIND_CODE) == 2);
static_assert(alignof(MulNX_UNWIND_CODE) == 2);

struct alignas(4) MulNX_UNWIND_INFO {
    uint8_t VersionFlags;        // 低 3 位 = Version, 高 5 位 = Flags
    uint8_t SizeOfProlog;        // prolog 长度（UBYTE，≤ 255）
    uint8_t CountOfCodes;        // unwind code slot 数
    uint8_t FrameRegOff;         // 低 4 位 = FrameRegister, 高 4 位 = FrameOffset
    MulNX_UNWIND_CODE UnwindCode[2];   // 目前使用 2 个 slot
};
static_assert(sizeof(MulNX_UNWIND_INFO) == 8);
static_assert(alignof(MulNX_UNWIND_INFO) == 4);

// UWOP 操作码（取自官方 UNWIND_CODE_OPS 枚举）
constexpr uint8_t MULNX_UWOP_PUSH_NONVOL = 0;
constexpr uint8_t MULNX_UWOP_ALLOC_LARGE = 1;
constexpr uint8_t MULNX_UWOP_ALLOC_SMALL = 2;
constexpr uint8_t MULNX_UWOP_SET_FPREG = 3;
constexpr uint8_t MULNX_UWOP_SAVE_NONVOL = 4;
constexpr uint8_t MULNX_UWOP_SAVE_NONVOL_FAR = 5;
constexpr uint8_t MULNX_UWOP_SAVE_XMM128 = 8;
constexpr uint8_t MULNX_UWOP_SAVE_XMM128_FAR = 9;
constexpr uint8_t MULNX_UWOP_PUSH_MACHFRAME = 10;

// Flags（与 winnt.h 的 UNW_FLAG_* 同值）
constexpr uint8_t MULNX_UNW_FLAG_NHANDLER = 0;

// RUNTIME_FUNCTION(12B) + UNWIND_INFO(8B)，4 字节对齐
static constexpr size_t kUnwindTableBytes =
sizeof(RUNTIME_FUNCTION) + sizeof(MulNX_UNWIND_INFO);