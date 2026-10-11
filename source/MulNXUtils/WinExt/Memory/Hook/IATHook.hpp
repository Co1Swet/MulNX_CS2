#pragma once
#include <MulNX/Common/Type.hpp>
#include <Windows.h>
#include <utility>

namespace MulNX::Memory {
    class IatHook {
        void** pSlot = nullptr;
        void* pOldFunc = nullptr;
        void* pNewFunc = nullptr;
        bool  attached = false;
    public:
        IatHook() = default;

        IatHook(void** slot, void* newFunc)
            : pSlot(slot), pNewFunc(newFunc) {
            this->pOldFunc = *this->pSlot;
        }

        ~IatHook() { this->Detach(); }

        // move-only
        IatHook(const IatHook&) = delete;
        IatHook& operator=(const IatHook&) = delete;

        IatHook(IatHook&& other) noexcept
            : pSlot(std::exchange(other.pSlot, nullptr))
            , pOldFunc(std::exchange(other.pOldFunc, nullptr))
            , pNewFunc(std::exchange(other.pNewFunc, nullptr))
            , attached(std::exchange(other.attached, false)) {}

        IatHook& operator=(IatHook&& other) noexcept {
            if (this != &other) {
                this->Detach();
                this->pSlot = std::exchange(other.pSlot, nullptr);
                this->pOldFunc = std::exchange(other.pOldFunc, nullptr);
                this->pNewFunc = std::exchange(other.pNewFunc, nullptr);
                this->attached = std::exchange(other.attached, false);
            }
            return *this;
        }

        // ------------------------------------------------------------------

        bool Attach() {
            if (!this->pSlot || !this->pNewFunc) return false;
            if (this->attached) return true;   // 已经接管，幂等

            DWORD oldProtect = 0;
            if (!VirtualProtect(this->pSlot, sizeof(void*), PAGE_READWRITE, &oldProtect))
                return false;
            *this->pSlot = this->pNewFunc;

            VirtualProtect(this->pSlot, sizeof(void*), oldProtect, &oldProtect);
            this->attached = true;
            return true;
        }

        void Detach() {
            if (!this->attached) return;       // 没接管过，什么都不做

            DWORD oldProtect = 0;
            if (VirtualProtect(this->pSlot, sizeof(void*), PAGE_READWRITE, &oldProtect)) {
                *this->pSlot = this->pOldFunc; // 恢复原值
                VirtualProtect(this->pSlot, sizeof(void*), oldProtect, &oldProtect);
            }
            this->attached = false;
        }

        // ------------------------------------------------------------------

        template<typename F>
        F Original() const { return reinterpret_cast<F>(this->pOldFunc); }

        void* OriginalRaw() const { return this->pOldFunc; }
        void** Slot()        const { return this->pSlot; }
        bool   IsAttached()  const { return this->attached; }
    };
}