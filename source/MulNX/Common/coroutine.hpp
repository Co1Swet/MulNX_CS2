#pragma once
#include <coroutine>
#include <atomic>
#include <exception>

namespace MulNX {
    class CoTask {
    public:
        struct promise_type {
            std::coroutine_handle<promise_type> hContinueCoroutine = nullptr;
            std::shared_ptr<std::exception_ptr> ppException = nullptr;

            CoTask get_return_object() {
                auto task = CoTask{ std::coroutine_handle<promise_type>::from_promise(*this) };
                this->ppException = task.ppException;
                return task;
            }

            void return_void() {}

            void unhandled_exception() {
                if (this->hContinueCoroutine == nullptr) {
                    // 自动销毁
                    throw;
                }
                *this->ppException = std::current_exception();
            }

            std::suspend_always initial_suspend() noexcept { return {}; }
            
            auto final_suspend() noexcept {
                struct FinalAwaitable {
                    
                    std::coroutine_handle<promise_type> hContinuation;
                    
                    bool await_ready() noexcept { return false; }

                    std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> hChild) noexcept {
                        std::coroutine_handle<> next = std::noop_coroutine();
                        if (this->hContinuation) {
                            next = this->hContinuation;
                        }
                        hChild.destroy();
                        return next;
                    }
                    void await_resume() noexcept {}
                };
                return FinalAwaitable{ this->hContinueCoroutine };
            }
        };
    private:
        std::coroutine_handle<promise_type> hCoroutine = nullptr;
        std::shared_ptr<std::exception_ptr> ppException = std::make_shared<std::exception_ptr>();
    public:
        bool await_ready() { return false; }

        std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> caller) {
            this->hCoroutine.promise().hContinueCoroutine = caller;
            return this->hCoroutine;   // 对称转移
        }

        void await_resume() {
            auto e = *this->ppException;
            if (!e)return;
            std::rethrow_exception(e);
        }

        explicit CoTask(std::coroutine_handle<promise_type> h) : hCoroutine(h) {}
        void Fire() { this->hCoroutine.resume(); }

        CoTask(const CoTask&) = delete;
        CoTask& operator=(const CoTask&) = delete;

        CoTask(CoTask&& other) noexcept {
            if (this == &other)return;
            this->hCoroutine = other.hCoroutine;
            this->ppException = other.ppException;
        }
        CoTask& operator=(CoTask&& other) noexcept {
            if (this == &other)return *this;
            this->hCoroutine = other.hCoroutine;
            this->ppException = other.ppException;
            return *this;
        }
    };
}