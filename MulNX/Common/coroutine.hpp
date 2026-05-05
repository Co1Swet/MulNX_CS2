// tiny_task.hpp
#pragma once
#include <coroutine>
#include <atomic>

struct Task {
    struct promise_type {
        Task get_return_object() {
            return Task{ std::coroutine_handle<promise_type>::from_promise(*this) };
        }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() { std::terminate(); }
    };

    std::coroutine_handle<promise_type> handle;

    // 默认构造函数：置空 handle
    Task() noexcept : handle(nullptr) {}
    explicit Task(std::coroutine_handle<promise_type> h) : handle(h) {}

    ~Task() { if (handle) handle.destroy(); }
    Task(const Task&) = delete;
    Task(Task&& other) noexcept : handle(other.handle) { other.handle = nullptr; }
    // ===== 加入移动赋值 =====
    Task& operator=(Task&& other) noexcept {
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

// 通用条件等待器
template<typename F>
struct AwaitCondition {
    F condition;

    bool await_ready() {
        return condition();
    }

    void await_suspend(std::coroutine_handle<>) {
        // 条件不满足，直接挂起，什么都不存
        // 恢复全凭外部调度器下一次 resume 这个协程
    }

    void await_resume() {}
};