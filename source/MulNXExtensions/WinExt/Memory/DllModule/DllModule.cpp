#include "DllModule.hpp"

#include <Windows.h>
#include <Psapi.h>
#pragma comment(lib, "Psapi.lib")

MulNX::Memory::DllModule::DllModule(std::wstring&& Name) : Name(std::move(Name)) {
    this->hModule = GetModuleHandleW(this->Name.c_str());
    if (!this->hModule) return;
    if (!GetModuleInformation(GetCurrentProcess(), this->hModule, &this->modInfo, sizeof(this->modInfo))) return;
    this->Valid = true;
}

uintptr_t MulNX::Memory::DllModule::GetBaseAddress()const {
    return reinterpret_cast<uintptr_t>(this->modInfo.lpBaseOfDll);
}
size_t MulNX::Memory::DllModule::GetModuleSize()const {
    return static_cast<size_t>(this->modInfo.SizeOfImage);
}

MulNX::Memory::Region MulNX::Memory::DllModule::GetTextRegion()const {
    if (!this->Valid) return Region::InValid();
    uintptr_t textStart = 0;
    size_t textSize = 0;
    if (!GetTextSectionRange(this->GetBaseAddress(), textStart, textSize)) {
        return Region::InValid();
    }
    return Region(textStart, textSize);
}

bool MulNX::Memory::GetModuleInfo(const wchar_t* moduleName, uintptr_t& baseAddr, size_t& moduleSize) {
    HMODULE hModule = GetModuleHandleW(moduleName);
    if (!hModule) return false;

    MODULEINFO modInfo = { 0 };
    if (!GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(modInfo)))
        return false;

    baseAddr = reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll);
    moduleSize = modInfo.SizeOfImage;
    return true;
}
bool MulNX::Memory::GetTextSectionRange(uintptr_t moduleBase, uintptr_t& textStart, size_t& textSize) {
    // DOS头
    IMAGE_DOS_HEADER* dosHeader = (IMAGE_DOS_HEADER*)moduleBase;
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) return false;

    // NT头
    IMAGE_NT_HEADERS* ntHeaders = (IMAGE_NT_HEADERS*)(moduleBase + dosHeader->e_lfanew);
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) return false;

    // 段表起始位置
    IMAGE_SECTION_HEADER* sections = IMAGE_FIRST_SECTION(ntHeaders);
    for (WORD i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++) {
        // 检查段是否包含可执行代码（IMAGE_SCN_MEM_EXECUTE 标志）
        if (sections[i].Characteristics & IMAGE_SCN_MEM_EXECUTE) {
            textStart = moduleBase + sections[i].VirtualAddress;
            textSize = sections[i].Misc.VirtualSize;
            return true;
        }
    }
    return false;
}