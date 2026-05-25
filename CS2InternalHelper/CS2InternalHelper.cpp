#include "CS2InternalHelper.hpp"
#include <CS2OBTool/DllMain/DllMain.hpp>
#include <filesystem>

HMODULE hOriginModule{};

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: {
        hOriginModule = hModule;
        break;
    }
    }
    return TRUE;
}

DWORD WINAPI HelperInit(void*) {
    WCHAR path[MAX_PATH] = { 0 };
    GetModuleFileNameW(hOriginModule, path, MAX_PATH);

    std::filesystem::path pthMeself(path);
    auto root = pthMeself.parent_path().parent_path();
    auto Tools = root / "Tools";
    auto dirFfmpeg = Tools / "ffmpeg";

    auto pthCS2OBTool = root / "CS2OBTool" / "CS2OBTool.dll";

    std::vector<std::string>ffmpegDllsName{
        "avutil-60.dll","swresample-6.dll","swscale-9.dll",
        "avcodec-62.dll","avformat-62.dll","avfilter-11.dll","avdevice-62.dll"
    };

    for (const auto& ffmpegDll : ffmpegDllsName) {
        auto fullPath = dirFfmpeg / ffmpegDll;
        auto result = LoadLibraryW(fullPath.wstring().c_str());
        if (!result) {
            MessageBoxW(NULL, L"Error", L"Error", MB_OK);
        }
    }

    auto result = LoadLibraryW(pthCS2OBTool.wstring().c_str());
    auto e = GetLastError();

    FARPROC pFunc = GetProcAddress(result, "MulNX_CS2_Start");
    reinterpret_cast<decltype(MulNX_CS2_Start)*>(pFunc)(nullptr);

    return 0;
}