#include "CS2Test.hpp"

bool CS2Test::Init() {
    std::thread([]() {
        MessageBoxW(NULL, L"MulNX 注入成功！", L"MulNX", MB_OK | MB_ICONINFORMATION);
        }).detach();
    this->AsyncCommand("playdemo 111");
    this->AsyncCommand("tv_listen_voice_indices -1");
    return true;
}