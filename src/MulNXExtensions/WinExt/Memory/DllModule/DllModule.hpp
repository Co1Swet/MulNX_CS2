#pragma once

#include "../Region/Region.hpp"

#include <cstdint>
#include <cstddef>
#include <string>
#include <Windows.h>
#include <Psapi.h>

#include <MulNX/Config/Config.hpp>

namespace MulNX {
    namespace Memory {
        class DllModule {
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

            template<typename F>
            typename MulNXFunc<F>::type GetProcAddressT(const std::string procName) const {
                FARPROC addr = GetProcAddress(this->hModule, procName.c_str());
                return reinterpret_cast<typename MulNXFunc<F>::type>(addr);
            }
        };
        bool GetModuleInfo(const wchar_t* moduleName, uintptr_t& baseAddr, size_t& moduleSize);
        bool GetTextSectionRange(uintptr_t moduleBase, uintptr_t& textStart, size_t& textSize);
    }
}