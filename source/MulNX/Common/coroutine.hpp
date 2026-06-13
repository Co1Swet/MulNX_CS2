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
            auto final_suspend() noexcept {
                struct FinalAwaitable {
                    bool await_ready() noexcept { return false; }
                    void await_suspend(std::coroutine_handle<promise_type> h) noexcept {
                        h.destroy();  // 自动销毁，协程帧释放
                    }
                    void await_resume() noexcept {}
                };
                return FinalAwaitable{};
            }
            void return_void() noexcept {}
            void unhandled_exception() { std::terminate(); }
        };

        std::coroutine_handle<promise_type> handle;
        bool started = false;

        CoTask() noexcept : handle(nullptr) {}
        explicit CoTask(std::coroutine_handle<promise_type> h) : handle(h) {}

        ~CoTask() { if (!this->started)this->handle.destroy(); }
        CoTask(const CoTask&) = delete;
        CoTask& operator=(CoTask& other) = delete;
        CoTask(CoTask&& other) = delete;
        CoTask& operator=(CoTask&& other) = delete;
        
        void resume() {
            this->started = true;
            this->handle.resume();
        }
    };
}