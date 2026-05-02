#pragma once

#include<Windows.h>
#include<filesystem>

class MulNXsPaths {
public:
    //std::filesystem::path GetPath();
    // 主路径结构
    struct S_MulNX {
        std::filesystem::path Path;

        struct S_MulNXConfig {
            std::filesystem::path Path;       // MulNXConfig文件
        } MulNXConfig;

        struct S_DLLToCS {
            std::filesystem::path Path;
            struct S_MulNXDLL_dll {
                std::filesystem::path Path;
            }MulNXDLL_dll;
        }CS2OBTool;

        struct S_Config {
            std::filesystem::path Path;
        } Config;

        struct S_MulNX_exe {
            std::filesystem::path Path;
        } MulNX_exe;

    } MulNX;

    struct cs2_exe {
        std::filesystem::path Path;
    } cs2_exe;
};

class MulNXeCore;

class IPCer {
    MulNXeCore* Core;
    MulNXsPaths Paths;
public:
    bool Inited = false;
    bool Init(MulNXeCore* Core);
    void VirtualMain();

    bool ITryedOpenCS2 = false;
    bool IOpenedCS2 = false;

    bool HadCS2Path = false;

    bool NeedToTryInject = false;
	bool Injected = false;

    bool HasCS2hProcess = false;

    long long CS2OpenTime = 0;
public:
    //由外部模块设置CS2的路径
    bool SetCS2Path(const std::filesystem::path& CS2Path);
    //通过对话框选择路径
    bool SelectCS2Path(); 
    //打开CS2
    bool OpenCS2();
    //尝试寻找CS2
    bool TryCatchCS2();
    //执行注入
    bool Inject();



    //CS2相关资源

    //CS2窗口句柄
    HWND CS2hWnd = nullptr;
    //CS2进程ID
    DWORD CS2PID = NULL;
    //CS2进程句柄
    HANDLE CS2hProcess = nullptr;


    //路径获取区如下

    std::filesystem::path PathGet_MulNX();
    std::filesystem::path PathGet_MulNXConfig();
    std::filesystem::path PathGet_CS2OBTool();
    std::filesystem::path PathGet_MulNXDLL_dll();

    std::filesystem::path PathGet_Config();

    std::filesystem::path PathGet_MulNX_exe();

    std::filesystem::path PathGet_cs2_exe();
};