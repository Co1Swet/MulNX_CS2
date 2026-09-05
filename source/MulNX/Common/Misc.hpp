#pragma once

template <typename F>
class scope_exit {
    F f;
public:
    explicit scope_exit(F&& func) : f(std::forward<F>(func)) {}
    ~scope_exit() { f(); }
    scope_exit(const scope_exit&) = delete;
    scope_exit& operator=(const scope_exit&) = delete;
};