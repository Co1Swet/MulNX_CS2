#pragma once

// Camera-only sampling result. The general float time adapter API stays unchanged.
namespace MulNX {
    enum class CameraTimeStatus {
        Legacy, Calibrating, Subtick, Paused, UnsupportedBuild, Unreadable,
        UnstableSnapshot, InvalidPhase, Discontinuity, TemporalMismatch, FrozenPhase, Busy
    };
    inline const char* CameraTimeStatusName(CameraTimeStatus status) noexcept {
        switch (status) {
        case CameraTimeStatus::Legacy: return "Legacy";
        case CameraTimeStatus::Calibrating: return "Calibrating (integer fallback)";
        case CameraTimeStatus::Subtick: return "Experimental sub-tick";
        case CameraTimeStatus::Paused: return "Paused sub-tick";
        case CameraTimeStatus::UnsupportedBuild: return "Unsupported build (integer fallback)";
        case CameraTimeStatus::Unreadable: return "Unreadable (integer fallback)";
        case CameraTimeStatus::UnstableSnapshot: return "Unstable snapshot (integer fallback)";
        case CameraTimeStatus::InvalidPhase: return "Invalid phase (integer fallback)";
        case CameraTimeStatus::Discontinuity: return "Seek/source reset (integer fallback)";
        case CameraTimeStatus::TemporalMismatch: return "Phase mismatch (integer fallback)";
        case CameraTimeStatus::FrozenPhase: return "Frozen phase (integer fallback)";
        default: return "Sampler busy (integer fallback)";
        }
    }
    struct CameraPathTime {
        double time = 0;
        bool requested = false, used = false;
        CameraTimeStatus status = CameraTimeStatus::Legacy;
    };
}
