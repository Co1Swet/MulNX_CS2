#include "CS2InternalHelper.hpp"
#include <CS2OBTool/DllMain/DllMain.hpp>
#include <filesystem>

HMODULE hOriginModule{};

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    hOriginModule = hModule;
    return TRUE;
}

DWORD WINAPI MulNX_HelperStart(void*) {
    auto hModule = GetModuleHandleW(L"CS2OBTool.dll");
    if (!hModule)return -1;

    FARPROC pFunc = GetProcAddress(hModule, "MulNX_CS2_Start");
    return reinterpret_cast<decltype(MulNX_CS2_Start)*>(pFunc)(nullptr);
}