#pragma once
#include <Windows.h>
extern "C" {
    __declspec(dllexport) DWORD WINAPI MulNX_HelperStart(void* msgPtr);
}