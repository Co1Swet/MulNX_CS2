#pragma once
#include <MulNXExtensions/TimeLiner/CameraPathTime.hpp>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace MulNX::ExperimentalCameraTime {
    // Byte-view only, NOT a replacement for the game's unverified CCSGlobalVars.
    struct GlobalsPhaseProbe { unsigned char unknown[0x38]; float phase; };
    static_assert(sizeof(float) == 4 && offsetof(GlobalsPhaseProbe, phase) == 0x38);
    inline constexpr uintptr_t PhaseOffset = offsetof(GlobalsPhaseProbe, phase);
    // Observed in the user-tested CS2 build. These identify
    // the sampled binaries, not a Valve-supported or semantically verified ABI.
    inline constexpr uint32_t ClientTimestamp = 1793836250, ClientImageSize = 43659264;
    inline constexpr uint32_t EngineTimestamp = 1793662660, EngineImageSize = 10571776;
    inline constexpr uintptr_t GlobalsSlotRva = 0x2235108;

    struct Input {
        int tick = -1;
        float alpha = 0;
        bool paused = false;
        uintptr_t demo = 0, globals = 0;
        uint64_t generation = 0;
        CameraTimeStatus validity = CameraTimeStatus::Subtick;
    };

    bool MatchesObservedBuild(uint32_t clientStamp, uint32_t clientSize,
        uint32_t engineStamp, uint32_t engineSize) noexcept;
    Input ReadProbe(uintptr_t clientBase, uintptr_t engineBase, uintptr_t globalsSlot,
        uintptr_t demo, int expectedTick, uint64_t generation) noexcept;

    class Sampler {
        Input previous{};
        bool havePrevious = false, ready = false, wrapped = false;
        unsigned increases = 0, unchanged = 0;
        void Seed(const Input& input) noexcept {
            previous = input; havePrevious = true; ready = wrapped = false;
            increases = unchanged = 0;
        }
    public:
        CameraPathTime Sample(const Input& input) noexcept {
            CameraPathTime result;
            result.time = double(input.tick) / 64.0;
            result.requested = true;
            result.status = input.validity;
            if (input.validity != CameraTimeStatus::Subtick || input.tick < 0) {
                havePrevious = ready = false;
                if (input.tick < 0) result.status = CameraTimeStatus::InvalidPhase;
                return result;
            }
            if (!std::isfinite(input.alpha) || input.alpha < 0 || input.alpha >= 1) {
                havePrevious = ready = false; result.status = CameraTimeStatus::InvalidPhase; return result;
            }
            if (!havePrevious || input.demo != previous.demo || input.globals != previous.globals ||
                input.generation != previous.generation) {
                Seed(input); result.status = CameraTimeStatus::Calibrating; return result;
            }
            const auto delta = int64_t(input.tick) - previous.tick;
            if (delta < 0 || delta > 1 || (previous.paused && input.paused && delta != 0)) {
                Seed(input); result.status = CameraTimeStatus::Discontinuity; return result;
            }
            if ((delta == 0 && input.alpha < previous.alpha) ||
                (input.paused && previous.paused && input.alpha != previous.alpha)) {
                Seed(input); result.status = CameraTimeStatus::TemporalMismatch; return result;
            }
            if (!input.paused) {
                unchanged = input.alpha == previous.alpha ? unchanged + 1 : 0;
                if (unchanged > 256) {
                    Seed(input); result.status = CameraTimeStatus::FrozenPhase; return result;
                }
                if (!previous.paused && delta == 0 && input.alpha > previous.alpha && increases < 8) ++increases;
                if (!previous.paused && delta == 1 && input.alpha < previous.alpha) wrapped = true;
                // Require both intra-tick movement and a tick-correlated wrap. A
                // merely readable 0..1 value must never immediately drive the path.
                ready = ready || (increases >= 8 && wrapped);
            }
            previous = input;
            if (ready) {
                result.time = (double(input.tick) + double(input.alpha)) / 64.0;
                result.used = true;
                result.status = input.paused ? CameraTimeStatus::Paused : CameraTimeStatus::Subtick;
            } else result.status = CameraTimeStatus::Calibrating;
            return result;
        }
    };
}
