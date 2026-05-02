#include "IPCer.hpp"

#include "../MulNXeCore.hpp"
#include "../ConfigManager/ConfigManager.hpp"

#include <Windows.h>
#include <ShObjIdl_core.h>
#include <shellapi.h>
#include <chrono>

bool IPCer::Init(MulNXeCore* Core) {
    this->Core = Core;
    HWND hwnd = FindWindowW(NULL, L"Multiple Next Extension");
    if (!hwnd) {
        return false; // 窗口未找到
    }

    DWORD processId;
    GetWindowThreadProcessId(hwnd, &processId);
    if (processId == 0) {
        return false;
    }

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
    if (!hProcess) {
        return false;
    }

    WCHAR exePath[MAX_PATH] = { 0 };
    DWORD size = MAX_PATH;

    // 获取可执行文件路径
    if (!QueryFullProcessImageNameW(hProcess, 0, exePath, &size)) {
        return false;
    }

    CloseHandle(hProcess);

    //设置MulNX.exe路径
    this->Paths.MulNX.MulNX_exe.Path = exePath;
    // 获取MulNX目录
    this->Paths.MulNX.Path = this->PathGet_MulNX_exe().parent_path();
    this->Paths.MulNX.MulNXConfig.Path = this->PathGet_MulNX() / "MulNXConfig";
    this->Paths.MulNX.CS2OBTool.Path = this->PathGet_MulNX() / "CS2OBTool";
    this->Paths.MulNX.CS2OBTool.MulNXDLL_dll.Path = this->PathGet_CS2OBTool() / "CS2OBTool.dll";
    this->Paths.MulNX.Config.Path = this->PathGet_MulNX() / "Config";

    this->Inited = true;
    return true;
}

std::filesystem::path IPCer::PathGet_MulNX() {
    return this->Paths.MulNX.Path;
}
std::filesystem::path IPCer::PathGet_MulNXConfig() {
    return this->Paths.MulNX.MulNXConfig.Path;
}
std::filesystem::path IPCer::PathGet_CS2OBTool() {
    return this->Paths.MulNX.CS2OBTool.Path;
}
std::filesystem::path IPCer::PathGet_MulNXDLL_dll() {
    return this->Paths.MulNX.CS2OBTool.MulNXDLL_dll.Path;
}

std::filesystem::path IPCer::PathGet_Config() {
    return this->Paths.MulNX.Config.Path;
}
std::filesystem::path IPCer::PathGet_MulNX_exe() {
    return this->Paths.MulNX.MulNX_exe.Path;
}

std::filesystem::path IPCer::PathGet_cs2_exe() {
    return this->Paths.cs2_exe.Path;
}


void IPCer::VirtualMain() {
    if (this->NeedToTryInject) {
        if (this->CS2hProcess) {
            long long now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
            long long delta = now - this->CS2OpenTime;
            long long Countdown = 20 - delta;
            static long long CountdownBuffer;
            if (CountdownBuffer != Countdown) {
                CountdownBuffer = Countdown;
            }
            if (Countdown <= 0) {
                if (this->Inject()) {
                    this->NeedToTryInject = false;
					this->Injected = true;
                }
            }
        }
        else {
            this->TryCatchCS2();
        }
    }
    /*if (this->Injected && this->DLLStarted == false) {
		if (this->StartDLL()) {
            this->DLLStarted = true;
        }
    }*/
}





bool IPCer::SetCS2Path(const std::filesystem::path& CS2Path) {
    this->Paths.cs2_exe.Path = CS2Path;

    return true;
}
bool IPCer::SelectCS2Path() { 
    //通过对话框选择路径
    
    //初始化COM库
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        return false;
    }

    IFileOpenDialog* pFileOpen = NULL;

    // 创建文件打开对话框对象
    hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
        IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

    if (SUCCEEDED(hr)) {
        // 显示对话框
        hr = pFileOpen->Show(NULL);

        if (SUCCEEDED(hr)) {
            IShellItem* pItem;
            hr = pFileOpen->GetResult(&pItem);

            if (SUCCEEDED(hr)) {
                PWSTR pszFilePath;
                hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                if (SUCCEEDED(hr)) {
                    // 输出文件路径
                    this->Paths.cs2_exe.Path = pszFilePath;

                    // 释放字符串内存
                    CoTaskMemFree(pszFilePath);
                }
                pItem->Release();
            }
        }
        pFileOpen->Release();
    }

    // 清理COM库
    CoUninitialize();
    if (std::filesystem::exists(this->Paths.cs2_exe.Path) && this->Paths.cs2_exe.Path.filename() == L"cs2.exe") {
        return true;
    }
    return false;
}
bool IPCer::OpenCS2() {
    //先检查是否有已经运行中的CS2实例
    HWND hwnd = FindWindowW(nullptr, L"Counter-Strike 2");
    if (hwnd) {
        //如果有则直接发出异常
        throw std::string("检测到运行中的CS2实例，请关闭后尝试重新打开");
    }
    //检查是否正在注入阶段中
    if (this->NeedToTryInject) {
        //如果是则直接发出异常
        throw std::string("正在进行注入，请不要重复注入");
    }
    //归位数据
    this->CS2hWnd = nullptr;
    this->CS2PID = NULL;
    this->CS2hProcess = nullptr;
    //可否进行打开标志位
    bool SafeOpen = true;

    //检查CS2路径是否为空
    if (this->Paths.cs2_exe.Path.empty()) {
        SafeOpen = false;
    }
    //检查CS2路径是否存在并检查是不是cs2.exe
    if (!(std::filesystem::exists(this->Paths.cs2_exe.Path) && this->Paths.cs2_exe.Path.filename() == L"cs2.exe")) {
        SafeOpen = false;
    }
    if (!SafeOpen) {
        //进行路径的重新选择
        SafeOpen = this->SelectCS2Path();
    }
    if (!SafeOpen) {
        return false;
    }
    this->Core->ConfigManager().Config_SetCS2Path(this->PathGet_cs2_exe());
    this->Core->ConfigManager().Config_Save(this->PathGet_Config());
    //构建参数
    std::wstring parameters = L"-insecure -worldwide -windowed -novid -allow_third_party_software";

    HINSTANCE result = ShellExecuteW(
        NULL,                                       // 父窗口句柄
        L"open",                                    // 操作
        this->Paths.cs2_exe.Path.c_str(),           // 应用程序路径
        parameters.c_str(),                         // 参数
        NULL,                                       // 工作目录
        SW_SHOWNORMAL                               // 显示方式
    );

    if (reinterpret_cast<INT_PTR>(result) > 32) {
        this->NeedToTryInject = true;
        return true;
    }
    return false;
}
bool IPCer::TryCatchCS2() {
    //检测是否已经有了进程句柄，有了则直接返回，代表已有
    if (this->CS2hProcess) {
        return true;
    }
    //寻找窗口，得到句柄
    this->CS2hWnd = FindWindowW(nullptr, L"Counter-Strike 2");
    if (!this->CS2hWnd) {
        //this->Core->Console().PrintInfor("未找到CS窗口！\n");
        return false;
    }
    //通过句柄得到进程ID
    GetWindowThreadProcessId(this->CS2hWnd, &this->CS2PID);
    if (!this->CS2PID) {
        return false;
    }
    //打开进程
    this->CS2hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, this->CS2PID);
    if (!this->CS2hProcess) {
        return false;
    }
    //这里因为获取到所有资源，则证明已经打开
    this->CS2OpenTime = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    return true;
}

bool IPCer::Inject() {
    if (this->Paths.MulNX.CS2OBTool.MulNXDLL_dll.Path.empty()) {
        return false;
    }
    if (!std::filesystem::exists(this->Paths.MulNX.CS2OBTool.MulNXDLL_dll.Path)) {
        return false;
    }
    if (!this->CS2hProcess || this->CS2hProcess == INVALID_HANDLE_VALUE) {
        return false;
    }

    //在目标进程分配内存
    LPVOID pRemoteMem = VirtualAllocEx(
        this->CS2hProcess,
        NULL,
        (this->Paths.MulNX.CS2OBTool.MulNXDLL_dll.Path.wstring().size() + 1) * sizeof(wchar_t),
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE
    );

    if (!pRemoteMem) {
        return false;
    }

    //写入DLL路径
    if (!WriteProcessMemory(
        this->CS2hProcess,
        pRemoteMem,
        this->Paths.MulNX.CS2OBTool.MulNXDLL_dll.Path.c_str(),
        (this->Paths.MulNX.CS2OBTool.MulNXDLL_dll.Path.wstring().size() + 1) * sizeof(wchar_t),
        NULL
    )) {
        VirtualFreeEx(this->CS2hProcess, pRemoteMem, 0, MEM_RELEASE);
        return false;
    }

    //获取LoadLibraryW地址
    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    if (!hKernel32) {
        return false;
    }

    FARPROC pLoadLibrary = GetProcAddress(hKernel32, "LoadLibraryW");
    if (!pLoadLibrary) {
        return false;
    }

    //创建远程线程
    HANDLE hThread = CreateRemoteThread(
        this->CS2hProcess,
        NULL,
        0,
        (LPTHREAD_START_ROUTINE)pLoadLibrary,
        pRemoteMem,
        0,
        NULL
    );

    if (!hThread) {
        VirtualFreeEx(this->CS2hProcess, pRemoteMem, 0, MEM_RELEASE);
        return false;
    }

    //等待线程执行完成
    WaitForSingleObject(hThread, INFINITE);

    //清理资源
    DWORD exitCode = 0;
    GetExitCodeThread(hThread, &exitCode);

    CloseHandle(hThread);
    VirtualFreeEx(this->CS2hProcess, pRemoteMem, 0, MEM_RELEASE);

    //检查是否加载成功
    //HMODULE 非零表示成功

    if (exitCode) {
        return true;
    }
    return false;
}

//bool IPCer::StartDLL() {
//    //获取MulNXDLL地址
//    HMODULE hMulNXDLL = GetModuleHandleW(L"CS2OBTool.dll");
//    if (!hMulNXDLL) {
//        return false;
//    }
//    // 获取MulNX_CS2_Start函数地址
//    FARPROC pMulNX_CS2_Start = GetProcAddress(hMulNXDLL, "MulNX_CS2_Start");
//    if (!pMulNX_CS2_Start) {
//        return false;
//    }
//    // 创建线程执行MulNX_CS2_Start函数
//    HANDLE hStartThread = CreateRemoteThread(
//        this->CS2hProcess,
//        NULL,
//        0,
//        (LPTHREAD_START_ROUTINE)pMulNX_CS2_Start,
//        NULL,
//        0,
//        NULL
//    );
//    if (!hStartThread) {
//        return false;
//    }
//    //等待线程执行完成
//    WaitForSingleObject(hStartThread, INFINITE);
//    CloseHandle(hStartThread);
//    return true;
//}