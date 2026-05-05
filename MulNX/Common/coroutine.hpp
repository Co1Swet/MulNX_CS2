// tiny_task.hpp
#pragma once
#include <coroutine>
#include <atomic>
#include <exception>

namespace MulNX {
    struct CoTask {
        struct promise_type {
            CoTask get_return_object() {
                return CoTask{ std::coroutine_handle<promise_type>::from_promise(*this) };
            }
            std::suspend_always initial_suspend() noexcept { return {}; }
            std::suspend_always final_suspend() noexcept { return {}; }
            void return_void() noexcept {}
            void unhandled_exception() { std::terminate(); }
        };

        std::coroutine_handle<promise_type> handle;

        CoTask() noexcept : handle(nullptr) {}
        explicit CoTask(std::coroutine_handle<promise_type> h) : handle(h) {}

        ~CoTask() { if (handle) handle.destroy(); }
        CoTask(const CoTask&) = delete;
        CoTask(CoTask&& other) noexcept : handle(other.handle) { other.handle = nullptr; }
        CoTask& operator=(CoTask&& other) noexcept {
            if (this != &other) {
                if (handle) handle.destroy(); // 先销毁自己持有的旧协程
                handle = other.handle;
                other.handle = nullptr;
            }
            return *this;
        }

        bool done() const { return handle.done(); }
        void resume() { if (!done()) handle.resume(); }
    };
}