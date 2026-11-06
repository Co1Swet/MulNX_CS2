#include "SubtickCameraTime.hpp"
#include <Windows.h>
#include <array>
#include <cstring>

namespace MulNX::ExperimentalCameraTime {
    namespace {
        bool SafeRead(uintptr_t address, void* destination, size_t bytes) noexcept {
            if (!address || !destination || !bytes || address > UINTPTR_MAX - bytes) return false;
            const auto end = address + bytes;
            for (auto p = address; p < end;) {
                MEMORY_BASIC_INFORMATION mbi{};
                if (!VirtualQuery(reinterpret_cast<void*>(p), &mbi, sizeof(mbi)) ||
                    mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS))) return false;
                const auto protection = mbi.Protect & 0xff;
                if (protection != PAGE_READONLY && protection != PAGE_READWRITE &&
                    protection != PAGE_WRITECOPY && protection != PAGE_EXECUTE_READ &&
                    protection != PAGE_EXECUTE_READWRITE && protection != PAGE_EXECUTE_WRITECOPY) return false;
                const auto next = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
                if (next <= p) return false;
                p = next;
            }
            __try {
                std::memcpy(destination, reinterpret_cast<const void*>(address), bytes);
                return true;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
        }
        bool Executable(uintptr_t address) noexcept {
            MEMORY_BASIC_INFORMATION mbi{};
            if (!VirtualQuery(reinterpret_cast<void*>(address), &mbi, sizeof(mbi)) ||
                mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS))) return false;
            const auto p = mbi.Protect & 0xff;
            return p == PAGE_EXECUTE || p == PAGE_EXECUTE_READ ||
                p == PAGE_EXECUTE_READWRITE || p == PAGE_EXECUTE_WRITECOPY;
        }
        // Only the same stateless CDemoPlayer methods already bound by TimeController.
        // No GetTime call, no engine writes. Catch stale-object/unmap faults as well.
        bool ReadDemoState(uintptr_t demo, int& tick, int& playing, int& paused) noexcept {
            uintptr_t vtable = 0;
            std::array<uintptr_t, 13> methods{};
            if (!SafeRead(demo, &vtable, sizeof(vtable)) ||
                !SafeRead(vtable, methods.data(), sizeof(methods)) ||
                !Executable(methods[3]) || !Executable(methods[11]) || !Executable(methods[12])) return false;
            __try {
                playing = reinterpret_cast<bool(*)(void*)>(methods[11])(reinterpret_cast<void*>(demo));
                paused = reinterpret_cast<bool(*)(void*)>(methods[12])(reinterpret_cast<void*>(demo));
                tick = reinterpret_cast<int(*)(void*)>(methods[3])(reinterpret_cast<void*>(demo));
                return true;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
        }
        bool ImageIdentity(uintptr_t base, uint32_t& stamp, uint32_t& size) noexcept {
            IMAGE_DOS_HEADER dos{};
            IMAGE_NT_HEADERS64 nt{};
            if (!SafeRead(base, &dos, sizeof(dos)) || dos.e_magic != IMAGE_DOS_SIGNATURE ||
                dos.e_lfanew <= 0 || dos.e_lfanew > 0x100000 || base > UINTPTR_MAX - dos.e_lfanew ||
                !SafeRead(base + dos.e_lfanew, &nt, sizeof(nt)) ||
                nt.Signature != IMAGE_NT_SIGNATURE || nt.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) return false;
            stamp = nt.FileHeader.TimeDateStamp; size = nt.OptionalHeader.SizeOfImage;
            return true;
        }
    }
    bool MatchesObservedBuild(uint32_t clientStamp, uint32_t clientSize,
        uint32_t engineStamp, uint32_t engineSize) noexcept {
        return clientStamp == ClientTimestamp && clientSize == ClientImageSize &&
            engineStamp == EngineTimestamp && engineSize == EngineImageSize;
    }
    Input ReadProbe(uintptr_t clientBase, uintptr_t engineBase, uintptr_t globalsSlot,
        uintptr_t demo, int expectedTick, uint64_t generation) noexcept {
        Input input;
        input.tick = expectedTick; input.demo = demo; input.generation = generation;
        uint32_t clientStamp = 0, clientSize = 0, engineStamp = 0, engineSize = 0;
        if (!ImageIdentity(clientBase, clientStamp, clientSize) || !ImageIdentity(engineBase, engineStamp, engineSize) ||
            !MatchesObservedBuild(clientStamp, clientSize, engineStamp, engineSize) ||
            clientBase > UINTPTR_MAX - GlobalsSlotRva || globalsSlot != clientBase + GlobalsSlotRva) {
            input.validity = CameraTimeStatus::UnsupportedBuild; return input;
        }
        int tickBefore = -1, playingBefore = -1, pausedBefore = -1;
        int tickAfter = -1, playingAfter = -1, pausedAfter = -1;
        if (!ReadDemoState(demo, tickBefore, playingBefore, pausedBefore) ||
            !SafeRead(globalsSlot, &input.globals, sizeof(input.globals)) ||
            !input.globals || input.globals > UINTPTR_MAX - PhaseOffset ||
            !SafeRead(input.globals + PhaseOffset, &input.alpha, sizeof(input.alpha))) {
            input.validity = CameraTimeStatus::Unreadable; return input;
        }
        if (!ReadDemoState(demo, tickAfter, playingAfter, pausedAfter)) {
            input.validity = CameraTimeStatus::Unreadable; return input;
        }
        uintptr_t globalsAfter = 0;
        if (!SafeRead(globalsSlot, &globalsAfter, sizeof(globalsAfter)) ||
            globalsAfter != input.globals || tickBefore != expectedTick || tickAfter != expectedTick ||
            playingBefore != 1 || playingAfter != 1 || pausedBefore != pausedAfter) {
            input.validity = CameraTimeStatus::UnstableSnapshot; return input;
        }
        input.paused = pausedAfter != 0;
        return input;
    }
}
