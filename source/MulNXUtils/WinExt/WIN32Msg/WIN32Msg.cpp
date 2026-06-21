#include "WIN32Msg.hpp"

bool MulNX::Win32::IsMouseMessage(UINT uMsg) {
	switch (uMsg) {
	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN: case WM_LBUTTONUP: case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN: case WM_RBUTTONUP: case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDOWN: case WM_MBUTTONUP: case WM_MBUTTONDBLCLK:
	case WM_XBUTTONDOWN: case WM_XBUTTONUP: case WM_XBUTTONDBLCLK:
	case WM_MOUSEWHEEL: case WM_MOUSEHWHEEL:
	case WM_NCMOUSEMOVE:
	case WM_NCLBUTTONDOWN: case WM_NCLBUTTONUP: case WM_NCLBUTTONDBLCLK:
	case WM_NCRBUTTONDOWN: case WM_NCRBUTTONUP: case WM_NCRBUTTONDBLCLK:
	case WM_NCMBUTTONDOWN: case WM_NCMBUTTONUP: case WM_NCMBUTTONDBLCLK:
	case WM_MOUSELEAVE: case WM_NCMOUSELEAVE:
		return true;
	default:
		return false;
	}
}
bool MulNX::Win32::IsKeyboardMessage(UINT uMsg) {
	switch (uMsg) {
	case WM_KEYDOWN: case WM_KEYUP: case WM_CHAR:
	case WM_SYSKEYDOWN: case WM_SYSKEYUP: case WM_SYSCHAR:
	case WM_DEADCHAR: case WM_SYSDEADCHAR:
	case WM_HOTKEY:
	case WM_APPCOMMAND:
	case WM_INPUTLANGCHANGE:
	case WM_SETFOCUS: case WM_KILLFOCUS:
		return true;
	default:
		return false;
	}
}