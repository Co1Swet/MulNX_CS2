#pragma once
#include <MulNX/Common/Type.hpp>
#include <MulNXUtils/WinExt/Memory/Region/Region.hpp>
#include <cstdint>
#include <cstddef>
#include <string>
#include <Windows.h>
#include <Psapi.h>

namespace MulNX::Memory {
    class DllModule {
        static bool GetModuleInfo(const wchar_t* moduleName, uintptr_t& baseAddr, size_t& moduleSize);
        static bool GetTextSectionRange(uintptr_t moduleBase, uintptr_t& textStart, size_t& textSize);
    public:
        bool Valid = false;
        HMODULE hModule = nullptr;
        MODULEINFO modInfo = { 0 };
        std::wstring Name{};

        DllModule() = default;
        DllModule(std::wstring&& Name);

        uintptr_t GetBaseAddress()const;
        size_t GetModuleSize()const;

        Region GetTextRegion()const;
        void** FindIatSlot(const char* dllName, const char* funcName)const;

        template<typename F>
        typename MulNXFunc<F>::type GetProcAddressT(const std::string procName) const {
            FARPROC addr = GetProcAddress(this->hModule, procName.c_str());
            return reinterpret_cast<typename MulNXFunc<F>::type>(addr);
        }
    };
}