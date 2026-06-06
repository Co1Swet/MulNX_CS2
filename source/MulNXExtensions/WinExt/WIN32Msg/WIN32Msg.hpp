#pragma once

#include <Windows.h>

namespace MulNX {
    namespace Win32 {
        //判断消息是否是鼠标消息
        bool IsMouseMessage(UINT uMsg);
        //判断消息是否是键盘消息
        bool IsKeyboardMessage(UINT uMsg);
        struct Msg4 {
            HWND hWnd;
            UINT uMsg;
            WPARAM wParam;
            LPARAM lParam;
        };
    }
}