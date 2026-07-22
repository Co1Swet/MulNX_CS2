#pragma once
#include <coroutine>
#include <atomic>
#include <exception>

namespace MulNX {
    class CoTask {
    public:
        struct promise_type {
            std::coroutine_handle<promise_type> continue_coroutine_handle = nullptr;
            CoTask get_return_object() {
                return CoTask{ std::coroutine_handle<promise_type>::from_promise(*this) };
            }
            std::suspend_always initial_suspend() noexcept { return {}; }
            void return_void() {}
            void unhandled_exception() { std::terminate(); }
            auto final_suspend() noexcept {
                struct FinalAwaitable {
                    std::coroutine_handle<> continuation;

                    bool await_ready() noexcept {
                        return false;
                    }
                    std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> handle) noexcept {
                        auto next = this->continuation ? this->continuation : std::noop_coroutine();
                        handle.destroy();
                        return next;
                    }
                    void await_resume() noexcept {}
                };
                return FinalAwaitable{ this->continue_coroutine_handle };
            }
        };

        std::coroutine_handle<promise_type> handle = nullptr;

        bool await_ready() { return false; }

        std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> caller) {
            this->handle.promise().continue_coroutine_handle = caller;
            auto next = this->handle;
            this->handle = nullptr;
            return next;   // 对称转移
        }

        void await_resume() { return; }

        explicit CoTask(std::coroutine_handle<promise_type> h) : handle(h) {}
        void Fire() {
            auto h = this->handle;
            this->handle = nullptr;
            return h.resume();
        }
        ~CoTask() {
            if (this->handle)
                this->handle.destroy();
        }

        CoTask(const CoTask&) = delete;
        CoTask& operator=(const CoTask&) = delete;

        CoTask(CoTask&& other) noexcept {
            if (this == &other)return;
            if (this->handle)this->handle.destroy();
            this->handle = other.handle;
            other.handle = nullptr;
        }
        CoTask& operator=(CoTask&& other) noexcept {
            if (this == &other)return *this;
            if (this->handle)this->handle.destroy();
            this->handle = other.handle;
            other.handle = nullptr;
            return *this;
        }
    };
}