#pragma once
#include <MulNXUtils/WinExt/Memory/Hook/Hook.hpp>
#include <mutex>
#include <set>

class RetEditor {
    std::set<uintptr_t>detected;
    std::set<uintptr_t>setted;
    std::mutex mutex;
public:
    void Render();
    bool Check(MulNX::Hook* hk, RegContext* ctx); // 返回true意味着处于setted
};