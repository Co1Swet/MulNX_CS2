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

    template<typename T>
    class co_task {
    public:
        class promise_type {
        public:
            std::coroutine_handle<> continue_coroutine_handle = nullptr;
            T value{};
            co_task get_return_object() {
                return co_task{ std::coroutine_handle<promise_type>::from_promise(*this) };
            }
            std::suspend_always initial_suspend() noexcept { return {}; }
            void return_value(T value) {
                this->value = std::move(value);
            }
            void unhandled_exception() { std::terminate(); }
            auto final_suspend() noexcept {
                struct FinalAwaitable {
                    std::coroutine_handle<> continuation;

                    bool await_ready() noexcept {
                        return false;  // 无等待者，直接完成
                    }
                    std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type>) noexcept {
                        return this->continuation ? this->continuation : std::noop_coroutine();   // 对称转移到父协程
                    }
                    void await_resume() noexcept {}
                };
                return FinalAwaitable{ continue_coroutine_handle };
            }
        };

        std::coroutine_handle<promise_type> handle;

        auto operator co_await() && noexcept {
            struct co_task_awaiter {
                std::coroutine_handle<promise_type> handle;

                bool await_ready() { return this->handle.done(); }

                std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) {
                    this->handle.promise().continue_coroutine_handle = caller;
                    return this->handle;   // 对称转移
                }

                T await_resume() {
                    auto val = std::move(this->handle.promise().value);
                    this->handle.destroy();
                    this->handle = nullptr;
                    return val;
                }

                ~co_task_awaiter() {
                    if (handle) {
                        handle.destroy();
                    }
                }
            };
            auto h = std::exchange(this->handle, {});
            return co_task_awaiter{ h };
        }

        
        explicit co_task(std::coroutine_handle<promise_type> h) : handle(h) {}
        ~co_task() {
            if (this->handle)this->handle.destroy();
        }

        co_task(const co_task&) = delete;
        co_task& operator=(const co_task&) = delete;

        co_task(co_task&& other) noexcept {
            if (this != &other) {
                if (this->handle) this->handle.destroy();  // 销毁当前帧
                this->handle = std::exchange(other.handle, {});
            }
        }
        co_task& operator=(co_task&& other) noexcept {
            if (this != &other) {
                if (this->handle) this->handle.destroy();  // 销毁当前帧
                this->handle = std::exchange(other.handle, {});
            }
            return *this;
        }
    };
}